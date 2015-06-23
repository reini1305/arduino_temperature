<?php
	$_SESSION['navigation']='research';
	include("../../head.php");
?>
<!-- BEGIN Site Content -->

<?php
  // Don't give away too much info
  //error_reporting(0);
//echo date('Y-m-d H:i:s') . " time zone: " . date_default_timezone_get() . "<br/>";

  include('tso.php');

  // Hourly stat parameters
  $h_max_days_to_show = 7;  // Stats over the last week
  $h_min_hour         = 0;  // Start from midnight
  $h_max_hour         = 24; // Include all readings until midnight
  $h_step_width_y     = 2;  // Increment ticks on temperature (y) axis by X
  $h_label_step_size  = 4;  // Show 1 tick (label) on x axis every X hours

  // Weekly stat parameters
  $w_office_hour_start   = 7;  // Include only readings from X...
  $w_office_hour_end     = 18; // ... until X
  $w_max_weeks_to_show   = 52; // Show stats over past X weeks
  $w_step_width_y        = 2;  // Increment ticks on temperature (y) axis by X
  $w_keep_detailed_weeks = 2; // Keep detailed logs for most recent X weeks


  //-----------------------------------------------------------------
  // Hourly stats

  // Load all readings
  $log = load_temperature_readings($log_file, $time_offset);
  // Reduce to hourly median
  $full_hourly_stats = get_hourly_stats($log);
  // Limit to X days
  list($h_labels, $h_values, $h_days_shown) = limit_hourly_stats($full_hourly_stats, $h_max_days_to_show, $h_min_hour, $h_max_hour, $h_label_step_size);

  // Scale plot Y axis
  $h_min_t = floor(min($h_values)) - 5;
  if (abs($h_min_t) % 2 == 1) // Prefer even steps
    $h_min_t++;
  $h_max_t = ceil(max($h_values)) + 5;
  
  $h_steps = floor(($h_max_t - $h_min_t) / (float)$h_step_width_y);

  //-----------------------------------------------------------------
  // Weekly stats
  list($w_labels, $w_min, $w_max, $w_mean, $weeks_shown) = get_weekly_stats($full_hourly_stats, $w_office_hour_start, $w_office_hour_end, $w_max_weeks_to_show, $week_stat_file);
  // Scale Y axis
  $w_min_t = floor(min($w_min)) - 5;
  if (abs($w_min_t) % 2 == 1) // Prefer even steps
    $w_min_t++;
  $w_max_t = ceil(max($w_max)) + 5;
  $w_steps = floor(($w_max_t - $w_min_t) / (float)$w_step_width_y);

  reduce_temperature_readings($full_hourly_stats, $log_file, $week_stat_file, $w_office_hour_start, $w_office_hour_end, $w_keep_detailed_weeks, $time_offset);
?>

  <script src="Chart.min.js" type="text/javascript"></script>
  <h1>LRS Office Temperature</h1>

  <h2>Median hourly temperature in &deg;C over the past <?php echo $h_days_shown; ?> day<?php echo ($h_days_shown > 1) ? 's' : ''; ?></h2>
  <div style="margin-left:2em;">
    <canvas id="h-plot" width="800" height="400"></canvas>
  </div>

  <h2>Weekly statistics during office hours (<?php printf("Mon-Fri, %d-%dh", $w_office_hour_start, $w_office_hour_end); ?>)</h2>
  <div id="w-legend" style="margin-left:3em;"></div>
  <div style="margin-left:2em;">
    <canvas id="w-plot" width="800" height="400"></canvas>
  </div>

  <script type="text/javascript">
    (function(){
      //-------------------------------------------------------------
      // Hourly stats chart
      var chart_data = {
        labels: [<?php for($i = 0; $i < count($h_labels); $i++) { if ($i > 0) { echo ','; } echo "'$h_labels[$i]'"; } ?>],
        datasets: [
            {
                label: "LRS Office Temperature",
                fillColor: "rgba(220,50,50,0.2)",
                strokeColor: "rgba(220,50,50,1)",
                pointColor: "rgba(220,50,50,1)",
                pointColor: "rgba(220,50,50,1)",
                pointStrokeColor: "#fff",
                pointHighlightFill: "#fff",
                pointHighlightStroke: "rgba(220,50,50,1)",
                data: [<?php for($i = 0; $i < count($h_values); $i++) { if ($i > 0) { echo ', '; } printf("'%.1f'", $h_values[$i]); } ?>],
                showTooltip: true
            }
          ]
        };
      var chart_options = {
          labelsFilter: function (value, index) {
              return (index) % <?php echo $h_label_step_size; ?>  !== 0;
          },
          pointDotRadius : 3,
          pointHitDetectionRadius : 0,
//          pointDot : false,
          scaleOverride: true, 
          scaleStartValue: <?php echo $h_min_t; ?>,
          scaleStepWidth: <?php echo $h_step_width_y; ?>, 
          scaleSteps: <?php echo $h_steps; ?>
        };
  // http://www.chartjs.org/docs/#line-chart-data-structure
  // http://stackoverflow.com/questions/25514802/how-to-add-an-on-click-event-to-my-line-chart-using-chart-js
  // Extension for x-axis "spacing"
  // https://github.com/leighquince/Chart.js

    var chart_ctx = document.getElementById("h-plot").getContext("2d");
    var temp_chart = new Chart(chart_ctx).Line(chart_data, chart_options);


    //---------------------------------------------------------------
    // Weekly Statistics
    var week_data = {
      labels: [<?php for($i = 0; $i < count($w_labels); $i++) { if ($i > 0) { echo ','; } echo "'$w_labels[$i]'"; } ?>],
      datasets: [
          {
              label: "Maximum Office Temperature",
              fillColor: "rgba(220,50,50,0.2)",
              strokeColor: "rgba(220,50,50,1)",
              pointColor: "rgba(220,50,50,1)",
              pointColor: "rgba(220,50,50,1)",
              pointStrokeColor: "#fff",
              pointHighlightFill: "#fff",
              pointHighlightStroke: "rgba(220,50,50,1)",
              data: [<?php for($i = 0; $i < count($w_max); $i++) { if ($i > 0) { echo ', '; } printf("'%.1f'", $w_max[$i]); } ?>]
          },
          {
              label: "Mean Office Temperature",
              fillColor: "rgba(255,100,50,0.2)",
              strokeColor: "rgba(255,100,50,1)",
              pointColor: "rgba(255,100,50,1)",
              pointColor: "rgba(255,100,50,1)",
              pointStrokeColor: "#fff",
              pointHighlightFill: "#fff",
              pointHighlightStroke: "rgba(255,100,50,1)",
              data: [<?php for($i = 0; $i < count($w_mean); $i++) { if ($i > 0) { echo ', '; } printf("'%.1f'", $w_mean[$i]); } ?>]
          },
          {
              label: "Minimum Office Temperature",
              fillColor: "rgba(30,30,205,.2)",
              strokeColor: "rgba(30,30,205,1)",
              pointColor: "rgba(30,30,205,1)",
              pointColor: "rgba(30,30,205,1)",
              pointStrokeColor: "#fff",
              pointHighlightFill: "#fff",
              pointHighlightStroke: "rgba(30,30,30,1)",
              data: [<?php for($i = 0; $i < count($w_min); $i++) { if ($i > 0) { echo ', '; } printf("'%.1f'", $w_min[$i]); } ?>]
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
      scaleStartValue: <?php echo $w_min_t; ?>, 
      scaleStepWidth: <?php echo $w_step_width_y; ?>, 
      scaleSteps: <?php echo $w_steps; ?>
    };
    var week_ctx = document.getElementById("w-plot").getContext("2d");
    var week_chart = new Chart(week_ctx).Line(week_data, week_options);
    document.getElementById("w-legend").innerHTML = week_chart.generateLegend();
    
  })();
  </script>

<!-- END Site Content -->
<?php include("../../footer.php"); ?>
