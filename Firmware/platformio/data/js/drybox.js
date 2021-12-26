// Shorthand for $( document ).ready()
$(function() {

	$('#sliderTemperature').on('input change', function(){	
			$("label[for='labelSetTemperature']").text("Target temperature: " + parseInt(this.value) +"°C");
		});

	$('#sliderHeaterTemperature').on('input change', function(){	
			$("label[for='labelSetHeaterTemperature']").text("Max heater temperature: " + parseInt(this.value) +"°C");
		});

	$('#sliderFanSpeed').on('input change', function(){	
			$("label[for='labelSetFanSpeed']").text("Fan speed: " + parseInt(this.value) +"%");
		});

	$('#btnSet').on('click', function(){
		var temperature = $("#sliderTemperature").val();
		var heater = $("#sliderHeaterTemperature").val();
		var fanspeed = $("#sliderFanSpeed").val();

		// Send 
		$.ajax({
		  url: "/set",
		  type: "get", //send it through get method
		  data: { 
		    temperature: temperature, 
		    heater: heater, 
		    fanspeed: fanspeed,
		  },
		  success: function(response) {
		    //Do Something
		    console.log(response);
		    update_status(false);
		  },
		  error: function(xhr) {
		    //Do Something to handle error
		  }
		});
	});

	$('#btnTurnOff').on('click', function(){
		// Send 
		$.ajax({
		  url: "/off",
		  type: "get", //send it through get method
		  success: function(response) {
		    console.log(response);
		    update_status(false);
		  },
		  error: function(xhr) {
		    //Do Something to handle error
		  }
		});
	});

	$('a#pop-temp').popover({
			    trigger : 'click',  
			    placement : 'top', 
			    html: true, 
			    title : 'Target Temperature',
			    content : 'This is the maximum/desired temperature we want inside the box.<br>\
			    			The system will try to reach and then maintain this temperature.',
            });


	$('a#pop-fan').popover({
			    trigger : 'click',  
			    placement : 'top', 
			    html: true, 
			    title : 'Fan speed',
			    content : 'Set the speed of the internal fan used to circulate the air inside the dry box.<br><br>\
			    			0% = Off<br>\
			    			50% = 50% of fan\'s maximum RPM<br>\
			    			100% = Full speed',
            });


	$('a#pop-heater').popover({
			    trigger : 'click',  
			    placement : 'top', 
			    html: true, 
			    title : 'Max heater temperature',
			    content : 'Maximum allowed temerature heater can reach.<br>This limit should be set to avoid damage to your filament or the box/heater itself.<br>\
			    			For example; Let\'s say that your heater could reach max temperature of 150°C. However this could start to melt your filament, melt the dry box or start a fire.<br>\
			    			In order to avoid this, we set a heater temperature limit to make sure heater never passes this set temperature during normal operation.<br>\
			    			This limit will also affect how long it takes to reach target temperature.',
            });
	// Finally update status
	update_status(true);
});


function update_status(rearm)
{
	var ajaxTime= new Date().getTime();

	$.ajax({
	url: "/status",
	type: "get", //send it through get method
	success: function(response) {
		console.log(response);

		var res = jQuery.parseJSON(response);

		$('#status-temp-target').text(res.target_temp_in);
		$('#status-fan-speed').text(res.fan_speed);
		$('#status-temp-inside').text(res.temp_in.toFixed(1));
		$('#status-temp-inside2').text(res.temp_in.toFixed(1));
		$('#status-temp-inside-target').text(res.target_temp_in.toFixed(1));
		$('#status-temp-heater').text(res.temp_heater.toFixed(0));
		$('#status-temp-heater-max').text(res.max_temp_heater.toFixed(0));
		$('#status-humidity-inside').text(res.humid_in.toFixed(0));
		$('#status-temp-outside').text(res.temp_out.toFixed(0));
		$('#status-humidity-outside').text(res.humid_out.toFixed(0));

		// Homed status
		if(res.status == 1) {
			$('#status-box').text('Dry box is on').addClass('badge-success').removeClass('badge-warning').removeClass('badge-danger');
		}
		else {
			$('#status-box').text('Dry box is off').addClass('badge-warning').removeClass('badge-success').removeClass('badge-danger');
		}

	},
	error: function(xhr) {
		$('#status-box').text('Dry box is unreachable!').addClass('badge-danger').removeClass('badge-success').removeClass('badge-warning');
	}
	}).done(function () {
    var totalTime = new Date().getTime()-ajaxTime;
    // Here I want to get the how long it took to load some.php and use it further
	});;

	if(rearm) {
		rearm_status();
	}
}

function rearm_status()
{
	setTimeout(function () {
		update_status(true);
	}, 2000);
}

