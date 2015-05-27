<!-- BEGIN Site Content -->


<?php
//LRS srv seems to be 2 hours ahead...
//echo date('Y-m-d H:i:s') . " time zone: " . date_default_timezone_get() . "<br/>";

// Don't give away too much info
error_reporting(0);

function array_median($array) {

  //if (true)
    //return max($array);
    //return array_sum($array)/count($array);

  // perhaps all non numeric values should filtered out of $array here?
  $iCount = count($array);
  if ($iCount == 0) {
    return -1;
    //throw new DomainException('Median of an empty array is undefined');
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

$file = 'temp.csv';

$handle = fopen($file, "r");
if ($handle) {
  $readings = array();
  while (($line = fgets($handle)) !== false) {
    $tokens = explode(";", $line);
    if (count($tokens) === 7) {
      $year = (int)$tokens[0];
      $month = (int)$tokens[1];
      $day = (int)$tokens[2];
      $hour = (int)$tokens[3];
      $minute = (int)$tokens[4];
      $second = (int)$tokens[5];
      $t = (float)$tokens[6];

      //$key = $year ."-". $month ."-". $day ."-". $hour ."-". $minute ."-". $second;
      $key = mktime($hour, $minute, $second, $month, $day, $year);
      $readings[$key] = $t;
    } elseif (count($tokens) === 3) {
      $key = (int)$tokens[0];
      $readings[$key] = (float)$tokens[1];

/*tested lrs srv timing:
      $year = date('Y', $key);
      $month = date('m', $key);
      $day = date('d', $key);
      $hour = date('H', $key);
      $minute = date('i', $key);
      $second = date('s', $key);

echo "Read " . $year . "-" . $month . "-" . $day . " " . $hour . ":" . $minute . ":" . $second . "<br/>";
$key = $key - 3600;
      $year = date('Y', $key);
      $month = date('m', $key);
      $day = date('d', $key);
      $hour = date('H', $key);
      $minute = date('i', $key);
      $second = date('s', $key);
echo "  -1 " . $year . "-" . $month . "-" . $day . " " . $hour . ":" . $minute . ":" . $second . "<br/>";
*/
    }
  }
  fclose($handle);

  // Sort by date
  ksort($readings);

  // Ensure we start at a proper hour
  $hour_label_step_size = 4;
  $dates = array_keys($readings);
  for ($i = 0; $i < count($dates); $i++) {
    $hour = date('H', $dates[$i]);
    if ($hour % $hour_label_step_size == 0)
      break;
    unset($readings[$dates[$i]]);
  }

  // Group hours & limit output
  $current_year = -1;
  $current_month = -1;
  $current_day = -1;
  $current_hour = -1;
  $tvals = array();
  $days_shown = 0;
  $max_days = 7;
  $dates = array_keys($readings);
  $temperatures = array_values($readings);
  $k = array();
  $v = array();
  for ($i = count($readings)-1; $i >= 0 && $days_shown <= $max_days; $i--) {
//    $tokens = explode("-", $dates[$i]);
//echo "process " . $dates[$i] ."<br/>";
//    if (count($tokens) === 6) {
//      $year = (int)$tokens[0];
//      $month = (int)$tokens[1];
//      $day = (int)$tokens[2];
//      $hour = (int)$tokens[3];
//      $minute = (int)$tokens[4];
//      $second = (int)$tokens[5];
    $year = (int)date('Y', $dates[$i]);
    $month = (int)date('n', $dates[$i]);
    $day = (int)date('j', $dates[$i]);
    $hour = (int)date('G', $dates[$i]);

    if ($hour !== $current_hour) {
      // Store temperature readings from current hour
      if (count($tvals) > 0) {
        //$k[] = $current_year . "-" . $current_month . "-" . $current_day . " " . $current_hour . "h";
        if ($day !== $current_day) {
          $wd = date('l', strtotime($current_year . "-" . $current_month . "-" . $current_day)); // or 'D' for 3 char
          $k[] = $wd . " " . $current_hour . "h";
        } else {
          $k[] = $current_hour . "h";
        }
        $v[] = array_median($tvals);
//echo $wd . " " . $current_hour .  " Median of " . count($tvals) . ": " . array_median($tvals) . "<br/>";
      }
      $current_hour = $hour;
      $tvals = array();
    }
    $tvals[] = $temperatures[$i];

    if ($day !== $current_day) {
      $current_day = $day;
      $days_shown++;
      if ($days_shown > $max_days)
        break;
    }

    if ($month !== $current_month)
      $current_month = $month;

    if ($year !== $current_year)
      $current_year = $year;
  }
  if (count($tvals) > 0 && $days_shown <= $max_days) {
    //$k[] = $current_year . "-" . $current_month . "-" . $current_day . " " . $current_hour . "h";
    $wd = date('l', strtotime($current_year . "-" . $current_month . "-" . $current_day)); // or 'D' for 3 char
    $k[] = $wd . " " . $current_hour . "h";
    $v[] = array_median($tvals);
//echo "Median of " . count($tvals) . ": " . array_median($tvals) . "<br/>";
  }
  $true_max_days = min($days_shown, $max_days);

//  $labels = array_reverse($k);
//  $values = array_reverse($v);
  $labels = array_reverse($k);
  $values = array_round(array_reverse($v), 1);

  // Y axis scaling
  $min_t = floor(min($values)) - 5;
  if (abs($min_t) % 2 == 1) // Prefer even steps
    $min_t++;
  $max_t = ceil(max($values)) + 5;
  $step_width = 2.0;
  $steps = floor(($max_t - $min_t) / $step_width);

  //------------------------------------------------------------------------
  // Week statistics
  $this_week = (int)date('W');
  $current_week = -1;
  $tvals = array();
  $kw = array();
  $vw_min = array();
  $vw_max = array();
  $vw_mean = array();
  $weeks_shown = 0;
  $max_weeks = 52;
  $office_hour_start = 7;
  $office_hour_end = 18;
  for ($i = count($readings)-1; $i >= 0 && $weeks_shown <= $max_weeks; $i--) {
    //$tokens = explode("-", $dates[$i]);
    //if (count($tokens) === 6) {
    //  $year = (int)$tokens[0];
    //  $month = (int)$tokens[1];
    //  $day = (int)$tokens[2];
    $year = (int)date('Y', $dates[$i]);
    $month = (int)date('n', $dates[$i]);
    $day = (int)date('j', $dates[$i]);
    $hour = (int)date('G', $dates[$i]);

    $week  = (int)date('W', mktime(0, 0, 0, $month, $day, $year));
    $week_day = date( "w", $dates[$i]);

    if ($week !== $current_week) {
      // Store temperature readings from current hour
      if (count($tvals) > 0) {
        $vw_min[] = min($tvals);
        $vw_max[] = max($tvals);
        $vw_mean[] = (float)array_sum($tvals) / (float)count($tvals); 
        if ($current_week === $this_week)
          $kw[] = "Current week";
        else
          $kw[] = "Week " . $current_week;
      }
      $current_week = $week;
      $tvals = array();
      $weeks_shown++;
      if ($weeks_shown > $max_weeks)
        break;
    }
    // week day: 0 => sunday to 6 => saturday
    if ($hour >= $office_hour_start && $hour <= $office_hour_end && $week_day > 0 && $week_day < 6)
      $tvals[] = $temperatures[$i];
  }
  if (count($tvals) > 0 && $weeks_shown <= $max_weeks) {
    $vw_min[] = min($tvals);
    $vw_max[] = max($tvals);
    $vw_mean[] = (float)array_sum($tvals) / (float)count($tvals); 
    if ($current_week === $this_week)
      $kw[] = "Current week";
    else
      $kw[] = "Week " . $current_week;
  }
//  $wlbl = array_reverse($kw);
//  $wmin = array_reverse($vw_min);
//  $wmax = array_reverse($vw_max);
//  $wmean = array_reverse($vw_mean);
  $wlbl = array_reverse($kw);
  $wmin = array_round(array_reverse($vw_min), 1);
  $wmax = array_round(array_reverse($vw_max), 1);
  $wmean = array_round(array_reverse($vw_mean), 1);

  // Y axis scaling
  $min_t_w = floor(min($wmin)) - 5;
  if (abs($min_t_w) % 2 == 1) // Prefer even steps
    $min_t_w++;
  $max_t_w = ceil(max($wmax)) + 5;
  $step_width_w = 2.0;
  $steps_w = floor(($max_t_w - $min_t_w) / $step_width_w);
?>

  <script src="Chart.min.js" type="text/javascript"></script>
  <h1>Mobile Vision Office Temperature</h1>

  <h2>Median hourly temperature in &deg;C over the past <?php echo $true_max_days ?> days</h2>
  <div style="margin-left:2em;">
    <canvas id="temp-plot" width="800" height="400"></canvas>
  </div>

  <h2>Weekly statistics during office hours (<?php printf("Mon-Fri, %d-%dh", $office_hour_start, $office_hour_end); ?>)</h2>
  <div id="week-legend" style="margin-left:3em;"></div>
  <div style="margin-left:2em;">
    <canvas id="week-plot" width="800" height="400"></canvas>
  </div>

  <script type="text/javascript">
    (function(){
      //-------------------------------------------------------------
      // Daily/Day-wise Temperature chart
      var chart_data = {
        labels: [<?php for($i = 0; $i < count($labels); $i++) { if ($i > 0) { echo ','; } echo "'$labels[$i]'"; } ?>],
        datasets: [
            {
                label: "Mobile Vision Lab Temperature",
                fillColor: "rgba(220,50,50,0.2)",
                strokeColor: "rgba(220,50,50,1)",
                pointColor: "rgba(220,50,50,1)",
                pointColor: "rgba(220,50,50,1)",
                pointStrokeColor: "#fff",
                pointHighlightFill: "#fff",
                pointHighlightStroke: "rgba(220,50,50,1)",
                data: [<?php for($i = 0; $i < count($values); $i++) { if ($i > 0) { echo ', '; } printf("'%.1f'", $values[$i]); } ?>],
                showTooltip: true
            }
          ]
        };
      var chart_options = {
          labelsFilter: function (value, index) {
              return (index) % <?php echo $hour_label_step_size; ?>  !== 0;
          },
          pointDotRadius : 3,
          pointHitDetectionRadius : 0,
//          pointDot : false,
          scaleOverride: true, 
          scaleStartValue: <?php echo $min_t; ?>,
          scaleStepWidth: <?php echo $step_width; ?>, 
          scaleSteps: <?php echo $steps; ?>
        };
  // http://www.chartjs.org/docs/#line-chart-data-structure
  // http://stackoverflow.com/questions/25514802/how-to-add-an-on-click-event-to-my-line-chart-using-chart-js
// Extension for x-axis "spacing"
// https://github.com/leighquince/Chart.js

    var chart_ctx = document.getElementById("temp-plot").getContext("2d");
    var temp_chart = new Chart(chart_ctx).Line(chart_data, chart_options);

    //---------------------------------------------------------------
    // Weekly Statistics
    var week_data = {
      labels: [<?php for($i = 0; $i < count($wlbl); $i++) { if ($i > 0) { echo ','; } echo "'$wlbl[$i]'"; } ?>],
      datasets: [
          {
              label: "Maximum Lab Temperature",
              fillColor: "rgba(220,50,50,0.2)",
              strokeColor: "rgba(220,50,50,1)",
              pointColor: "rgba(220,50,50,1)",
              pointColor: "rgba(220,50,50,1)",
              pointStrokeColor: "#fff",
              pointHighlightFill: "#fff",
              pointHighlightStroke: "rgba(220,50,50,1)",
              data: [<?php for($i = 0; $i < count($wmax); $i++) { if ($i > 0) { echo ', '; } printf("'%.1f'", $wmax[$i]); } ?>]
          },
          {
              label: "Mean Lab Temperature",
              fillColor: "rgba(255,100,50,0.2)",
              strokeColor: "rgba(255,100,50,1)",
              pointColor: "rgba(255,100,50,1)",
              pointColor: "rgba(255,100,50,1)",
              pointStrokeColor: "#fff",
              pointHighlightFill: "#fff",
              pointHighlightStroke: "rgba(255,100,50,1)",
              data: [<?php for($i = 0; $i < count($wmean); $i++) { if ($i > 0) { echo ', '; } printf("'%.1f'", $wmean[$i]); } ?>]
          },
          {
              label: "Minimum Lab Temperature",
              fillColor: "rgba(30,30,205,.2)",
              strokeColor: "rgba(30,30,205,1)",
              pointColor: "rgba(30,30,205,1)",
              pointColor: "rgba(30,30,205,1)",
              pointStrokeColor: "#fff",
              pointHighlightFill: "#fff",
              pointHighlightStroke: "rgba(30,30,30,1)",
              data: [<?php for($i = 0; $i < count($wmin); $i++) { if ($i > 0) { echo ', '; } printf("'%.1f'", $wmin[$i]); } ?>]
          }
        ]
      };
    var week_options = {
      legendTemplate : '<b>Legend:</b><br/><ul style=\"margin-top:0em; padding-left:1em;\">'
                  +'<% for (var i=0; i<datasets.length; i++) { %>'
                    +'<li style=\"list-style-type:none;\">'
                    +'<span style=\"color:<%= datasets[i].strokeColor %>\">'
                    +'<% if (datasets[i].label) { %><%= datasets[i].label %><% } %>'
                  +'</span></li>'
                +'<% } %>'
              +'</ul>',
      pointHitDetectionRadius : 1,
      scaleOverride: true, 
      scaleStartValue: <?php echo $min_t_w; ?>, 
      scaleStepWidth: <?php echo $step_width_w; ?>, 
      scaleSteps: <?php echo $steps_w; ?>
    };
    var week_ctx = document.getElementById("week-plot").getContext("2d");
    var week_chart = new Chart(week_ctx).Line(week_data, week_options);
    document.getElementById("week-legend").innerHTML = week_chart.generateLegend();
  })();
  </script>  
<?php
} else {
    // error opening the file.
  echo "Cannot access temperature readings";
}
?>

