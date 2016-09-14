Pebble.addEventListener("ready",
    function(e) {
        Pebble.getTimelineToken(function(token) {
          console.log('My timeline token is ' + token);
        }, function(error) {
            console.log('Error getting timeline token: ' + error);
        });
    }
);
