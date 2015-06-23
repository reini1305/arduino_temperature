<?php
//-------------------------------------------------------------
// "Global parameters"
// Where to get the data from
$log_file       = 'temp_readings.txt'; // detailed log file
$week_stat_file = 'weekly_stats.txt';  // reduced weekly statistics
$time_offset = 0;  //-7200; // deprecated: LRS srv seems to be 2hrs ahead, so date_offset is -7200 seconds

//-------------------------------------------------------------
// Array utilities

function array_median($array) {
  // perhaps all non numeric values should filtered out of $array here?
  $iCount = count($array);
  if ($iCount == 0) {
    return -1;
  }
  // if we're down here it must mean $array has at least 1 item in the array.
  $middle_index = floor($iCount / 2);
  sort($array, SORT_NUMERIC);
  $median = $array[$middle_index]; // assume an odd # of items
  // Handle the even case by averaging the middle 2 items
  if ($iCount % 2 == 0) {
    $median = ($median + $array[$middle_index - 1]) / 2.0;
  }
  return $median;
}

function array_round($array, $precision) {
  $val = array();
  foreach ($array as $v) {
    $val[] = round($v, $precision);
  }
  return $val;
}


//-------------------------------------------------------------
// Temperature loading/display utilities

function load_temperature_readings($log_file, $date_offset)
{
  $log = array();
  $handle = fopen($log_file, "r");
  if ($handle) {

    while (($line = fgets($handle)) !== false) {
      $tokens = explode(";", $line);
      if (count($tokens) === 2 || count($tokens) === 3) {
        $key = (int)$tokens[0] + $date_offset;
        $log[$key] = (float)$tokens[1];
      }
    }
    fclose($handle);
  }
  return $log;
}

// Log is obtained via load_temperature_readings
//   key is the timestamp, value is the temperature
function get_hourly_stats($log)
{
  // Sort by date
  ksort($log);

  // Group hours
  $current_year  = -1;
  $current_month = -1;
  $current_day   = -1;
  $current_hour  = -1;
  $current_temperatures = array();

  $timestamps   = array_keys($log);
  $temperatures = array_values($log);
  $stats = array();
  for ($i = count($log)-1; $i >= 0; $i--) {
    $year  = (int)date('Y', $timestamps[$i]);
    $month = (int)date('n', $timestamps[$i]);
    $day   = (int)date('j', $timestamps[$i]);
    $hour  = (int)date('G', $timestamps[$i]);
//echo "$timestamps[$i], $year, $month, $day<br/>";

    if ($hour !== $current_hour) {
      // Store temperature values for current hour
      if (count($current_temperatures) > 0) {
        $k = mktime($current_hour, 0, 0, $current_month, $current_day, $current_year);
        $stats[$k] = array_median($current_temperatures);
      }
      $current_hour = $hour;
      $current_temperatures = array();
    }
    $current_temperatures[] = $temperatures[$i];

    if ($day !== $current_day)
      $current_day = $day;

    if ($month !== $current_month)
      $current_month = $month;

    if ($year !== $current_year)
      $current_year = $year;
  }
  if (count($current_temperatures) > 0) {
    $k = mktime($current_hour, 0, 0, $current_month, $current_day, $current_year);
    $stats[$k] = array_median($current_temperatures);
//echo "LAST HOUR: $k, $current_year<br/>";
  }

  return $stats;
}

// full_hourly_stats (everything loaded from the log file), already sorted
function limit_hourly_stats($full_hourly_stats, $max_days_to_show, $min_hour, $max_hour, $label_step_size) {
  $hourly_stats  = array();

  $current_year  = -1;
  $current_month = -1;
  $current_day   = -1;
  $current_hour  = -1;
  $days_shown    = 0;

  $timestamps   = array_keys($full_hourly_stats);
  $temperatures = array_values($full_hourly_stats);
  $hour_changed = false;
  $day_changed  = false;

  for ($i = 0; $i < count($timestamps) && $days_shown <= $max_days_to_show; $i++) {
    $year  = (int)date('Y', $timestamps[$i]);
    $month = (int)date('n', $timestamps[$i]);
    $day   = (int)date('j', $timestamps[$i]);
    $hour  = (int)date('G', $timestamps[$i]);

    if ($year !== $current_year || $month !== $current_month || $day !== $current_day)
      $day_changed = true;
    else
      $day_changed = false;

    if ($day_changed)
      $days_shown++;

    $current_year  = $year;
    $current_month = $month;
    $current_day   = $day;
    $current_hour  = $hour;

    if ($days_shown <= $max_days_to_show && $hour >= $min_hour && $hour <= $max_hour)
      $hourly_stats[$timestamps[$i]] = $temperatures[$i];
  }

  $timestamps   = array_reverse(array_keys($hourly_stats));
  $temperatures = array_reverse(array_round(array_values($hourly_stats),1));

  $labels = array();
  $values = array();
  $current_day = -1;
  $current_labels = array();
  $current_values = array();
  for ($i = 0; $i < count($timestamps); $i++) {
    $day = (int)date('j', $timestamps[$i]);
    $hour  = (int)date('G', $timestamps[$i]);

    if ($day !== $current_day) {
      if (count($current_labels) > 0) {
        $skip = count($current_labels) % $label_step_size;  // Skip those (excessive) entries which would cause the plot label to shift...
        for ($j = 0; $j < count($current_labels)-$skip; $j++) { 
          $labels[] = $current_labels[$j];
          $values[] = $current_values[$j];
        }
        $current_labels = array();
        $current_values = array();
      }
      $current_labels[] = date('l',$timestamps[$i]) . " " . $hour . "h";
    } else {
      $current_labels[] = $hour . "h";
    }
    $current_values[] = $temperatures[$i];
    $current_day = $day;
  }
  // Current day is missed by loop
  if (count($current_labels) > 0) {
    for ($j = 0; $j < count($current_labels); $j++) {
      $labels[] = $current_labels[$j];
      $values[] = $current_values[$j];
    }
  }

  return array($labels, $values, min($days_shown, $max_days_to_show));
}

function weekly_stats($full_hourly_stats, $office_hour_start, $office_hour_end) {
  $timestamps = array_keys($full_hourly_stats);
  $temperatures = array_values($full_hourly_stats);

  $current_week = -1;
  $current_year = -1;
  $keys   = array();
  $w_min  = array();
  $w_max  = array();
  $w_mean = array();
  $tvals  = array();
  for ($i = 0; $i < count($timestamps); $i++) {
    $year  = (int)date('Y', $timestamps[$i]);
    $month = (int)date('n', $timestamps[$i]);
    $day   = (int)date('j', $timestamps[$i]);
    $hour  = (int)date('G', $timestamps[$i]);

//    $week  = (int)date('W', mktime(0, 0, 0, $month, $day, $year));
    $week = (int)date('W', $timestamps[$i]);
    $week_day = (int)date("w", $timestamps[$i]);

    if ($week !== $current_week || $year !== $current_year) {
      // Store temperature readings from current week
      if (count($tvals) > 0) {
        $w_min[] = min($tvals);
        $w_max[] = max($tvals);
        $w_mean[] = (float)array_sum($tvals) / (float)count($tvals); 
        $keys[] = $current_week + $current_year * 100;
        //echo "$week($current_week) of $year/$current_year = $timestamps[$i]<br/>";
      }
      $current_year = $year;
      $current_week = $week;
      $tvals = array();
    }
    // week day: 0 => sunday to 6 => saturday
    if ($hour >= $office_hour_start && $hour <= $office_hour_end && $week_day > 0 && $week_day < 6)
      $tvals[] = $temperatures[$i];
  }
  if (count($tvals) > 0) {
    $w_min[] = min($tvals);
    $w_max[] = max($tvals);
    $w_mean[] = (float)array_sum($tvals) / (float)count($tvals); 
    $keys[] = $current_week + $current_year * 100;
  }
  return array($keys, $w_min, $w_max, $w_mean);
}

function get_weekly_stats($full_hourly_stats, $office_hour_start, $office_hour_end, $max_weeks_to_show, $week_stat_file) {
  list($keys, $w_min, $w_max, $w_mean) = weekly_stats($full_hourly_stats, $office_hour_start, $office_hour_end);
  list($reduced_keys, $reduced_min, $reduced_max, $reduced_mean) = load_reduced_temperature_readings($week_stat_file);
//var_dump($reduced_keys);
  $c_keys = array_merge($keys, $reduced_keys);
  $c_min  = array_merge($w_min, $reduced_min);
  $c_max  = array_merge($w_max, $reduced_max);
  $c_mean = array_merge($w_mean, $reduced_mean);

  // Sort by year*100+week (keep associative idx)
  arsort($c_keys);
//var_dump($c_keys);echo "<br/><br/>";

  $weeks_shown = 0;
  $labels = array();
  $v_min  = array();
  $v_max  = array();
  $v_mean = array();
  
  // Limit & get proper labels
  $idx = array_keys($c_keys);
  for ($i = 0; $i < count($c_keys) && $weeks_shown < $max_weeks_to_show; $i++, $weeks_shown++) {
    $j = $idx[$i];
    $week = $c_keys[$j] % 100;
    if ($i === 0)
      $labels[] = "Current week";
    else
      $labels[] = "Week " . $week;
    $v_min[]  = $c_min[$j];
    $v_max[]  = $c_max[$j];
    $v_mean[] = $c_mean[$j];
  }
//var_dump($labels); echo "<br/><br/>";
  return array(array_reverse($labels), array_round(array_reverse($v_min),1), array_round(array_reverse($v_max),1), array_round(array_reverse($v_mean),1), $weeks_shown);
}

function reduce_temperature_readings($full_hourly_stats, $log_file, $week_file, $office_hour_start, $office_hour_end, $keep_num_weeks, $date_offset) {
  list($keys, $w_min, $w_max, $w_mean) = weekly_stats($full_hourly_stats, $office_hour_start, $office_hour_end);

  // Find date up until which everything should be reduced  
  $this_week = (int)date('W');
  $this_year = (int)date('Y');

  $target_year = $this_year; 
  $target_week = $this_week - $keep_num_weeks;
  if ($target_week < 1) {
    $target_year--;
    $target_week += 52;
  }
  $target_date = $target_week + $target_year * 100;

  // Store reduced week stats
  $any_reduced = false;
  for ($i = count($keys)-1; $i >= 0; $i--) {
    if ($keys[$i] <= $target_date) {
      $any_reduced = true;
      $week = $keys[$i] % 100;
      $year = (int)(($keys[$i] - $week)/100);
      $str = $year . ";" . $week . ";" . $w_min[$i] . ";" . $w_mean[$i] . ";" . $w_max[$i] . PHP_EOL;
      //echo "week stat: " . $str . " wtf: " . $keys[$i] . "<br/>";
      file_put_contents($week_file, $str, FILE_APPEND | LOCK_EX);
    }
  }

  // Rewrite log-file (keep only newer dates)
  $timestamps = array_keys($full_hourly_stats);
  $temperatures = array_values($full_hourly_stats);
  if ($any_reduced) {
    $first_put = false;
    for ($i = count($timestamps) - 1; $i >= 0; $i--) {
      $year = (int)date('Y', $timestamps[$i]);
      $week  = (int)date('W', $timestamps[$i]);
      $v = $week + $year * 100;
      if ($v > $target_date) {
        $d = $timestamps[$i] - $date_offset;
        $str = $d . ";" . $temperatures[$i] . PHP_EOL;
        //echo "Keeping $v  = $str<br/>";
        if ($first_put) {
          file_put_contents($log_file, $str, FILE_APPEND | LOCK_EX);
        } else {
          file_put_contents($log_file, $str, LOCK_EX);
          $first_put = true;
        }
      }
    }
  }
}


function load_reduced_temperature_readings($reduced_file) {
  $kw = array();
  $vw_min = array();
  $vw_max = array();
  $vw_mean = array();
  $handle = fopen($reduced_file, "r");
  if ($handle) {
    while (($line = fgets($handle)) !== false) {
      $tokens = explode(";", $line);
      if (count($tokens) === 5) {
        $year = (int)$tokens[0];
        $week = (int)$tokens[1];
        $vw_min[] = (float)$tokens[2];
        $vw_mean[] = (float)$tokens[3];
        $vw_max[] = (float)$tokens[4];
        $kw[] = $week + $year * 100;
      } 
    }
    fclose($handle);
  }

  return array($kw,$vw_min,$vw_max,$vw_mean);
}

?>
