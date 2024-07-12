var client = null;
function startConnect(){

    clientID = "sc_sim-"+parseInt(Math.random() * 100);

    host = document.getElementById("host").value;   
    port = document.getElementById("port").value;  

    log_str = "<span> Connecting to <b>" + host + "</b> on port <b>" +port+"</b> using client Id <b>"+clientID+"</b></span><br>";
    document.getElementById("messages").innerHTML += log_str;
    
    client = new Paho.MQTT.Client(host,Number(port),clientID);

    client.onConnectionLost = onConnectionLost;
    client.onMessageArrived = onMessageArrived;

    client.connect({
        onSuccess: onConnect,
        onFailure: function (error) {
            document.getElementById("messages").innerHTML += "<span> Connection failed: "+error.errorMessage+"</span><br>";
        }
    });

}


function onConnect(){
    topic =  document.getElementById("topic_s").value;

    document.getElementById("messages").innerHTML += "<span> Subscribing to topic <b>"+topic+"</b></span><br>";

    client.subscribe(topic);
    client.subscribe("basecamp/sc_sim/mgmt");
    client.subscribe("basecamp/sc_sim/model");
    client.subscribe("basecamp/sc_sim/event_plbk");
    client.subscribe("basecamp/sc_sim/event");
}


function onConnectionLost(responseObject){
    document.getElementById("messages").innerHTML += "<span> ERROR: Connection is lost.</span><br>";
    if(responseObject !=0){
        document.getElementById("messages").innerHTML += "<span> ERROR:"+ responseObject.errorMessage +"</span><br>";
    }
}

function onMessageArrived(message){
    console.log("OnMessageArrived: "+message.payloadString);
    //document.getElementById("messages").innerHTML += "<span> Topic:"+message.destinationName+"| Message : "+message.payloadString + "</span><br>";
    if (message.destinationName == "basecamp/sc_sim/mgmt"){
       const mgmt = JSON.parse(message.payloadString);
       document.getElementById("sim_active").value          = boolStr(mgmt.sim_active);
       document.getElementById("sim_time").value            = mgmt.sim_time;
       document.getElementById("time_before_contact").value = mgmt.contact_time_pending;
       document.getElementById("contact_length").value      = mgmt.contact_length;
       document.getElementById("contact_consumed").value    = mgmt.contact_time_consumed;
       document.getElementById("contact_remaining").value   = mgmt.contact_time_remaining;
    }
    if (message.destinationName == "basecamp/sc_sim/model"){
       //document.getElementById("messages").innerHTML += "<span> Topic:"+message.destinationName+"| Message : "+message.payloadString + "</span><br>";
       const model = JSON.parse(message.payloadString);
 
       mode = 'Undefined';
       switch(model.adcs_mode) {
       case 1:
          mode = 'Safehold';
          break;
       case 2:
          mode = 'Sun Point';
          break;
       case 3:
          mode = 'Interial';
          break;
       case 4:
          mode = 'Slew';
          break;
       }      
       document.getElementById("adcs_mode").value        = mode;
       document.getElementById("adcs_eclipse").value     = boolStr(model.adcs_eclipse);

       document.getElementById("cdh_sbc_resets").value   = model.cdh_sbc_rst_cnt;
       document.getElementById("cdh_hw_cmds").value      = model.cdh_hw_cmd_cnt;

       document.getElementById("comm_tdrs").value        = model.comm_contact_tdrs_id;
       document.getElementById("comm_data_rate").value   = model.comm_contact_data_rate;

       document.getElementById("fsw_rec_plbk").value     = boolStr(model.fsw_rec_playback_ena);
       document.getElementById("fsw_rec_file_cnt").value = model.fsw_rec_file_cnt;

       document.getElementById("instr_power").value      = boolStr(model.instr_instr_pwr_ena);
       document.getElementById("instr_science").value    = boolStr(model.instr_instr_sci_ena);

       document.getElementById("power_batt_soc").value   = model.power_batt_soc;
       document.getElementById("power_sa_curr").value    = model.power_sa_current;

       document.getElementById("therm_htr_1").value      = boolStr(model.therm_heater1_ena);
       document.getElementById("therm_htr_2").value      = boolStr(model.therm_heater2_ena);

    }
    if (message.destinationName == "basecamp/sc_sim/event_plbk"){
       //document.getElementById("messages").innerHTML += "<span> Playback: "+message.payloadString+"</span><br>";
       const plbk = JSON.parse(message.payloadString);

       document.getElementById("playback_event_messages").innerHTML = 
           "<span>"+plbk.event1_time+" ["+plbk.event1_app+":"+eventTypeStr(plbk.event1_type)+"] - "+plbk.event1_msg+"</span><br>" +
           "<span>"+plbk.event2_time+" ["+plbk.event2_app+":"+eventTypeStr(plbk.event2_type)+"] - "+plbk.event2_msg+"</span><br>" +
           "<span>"+plbk.event3_time+" ["+plbk.event3_app+":"+eventTypeStr(plbk.event3_type)+"] - "+plbk.event3_msg+"</span><br>" +
           "<span>"+plbk.event4_time+" ["+plbk.event4_app+":"+eventTypeStr(plbk.event4_type)+"] - "+plbk.event4_msg+"</span><br>";
    }
    if (message.destinationName == "basecamp/sc_sim/event"){
       const event_msg = JSON.parse(message.payloadString);
       
       document.getElementById("event_messages").innerHTML += "<span>"+event_msg.time+" ["+event_msg.app+":"+eventTypeStr(event_msg.type)+"] - "+event_msg.text+"</span><br>";

    }
    
}

function startDisconnect(){
    client.disconnect();
    document.getElementById("messages").innerHTML += "<span> Disconnected. </span><br>";
}

function publishMessage(){
   msg = document.getElementById("Message").value;
   topic = document.getElementById("topic_p").value;
   document.getElementById("messages").innerHTML += "<span> Preparing "+topic+":"+msg+"</span><br>";
   Message = new Paho.MQTT.Message(msg);
   Message.destinationName = topic;

   client.send(Message);
   document.getElementById("messages").innerHTML += "<span> Message to topic "+topic+" is sent </span><br>";

}

function clearActivityLog(){
    document.getElementById("messages").innerHTML = ""
}

function sendScSimCommand(cmd_id){

   const payload = '{ "id": '+cmd_id+' }' 
   Message = new Paho.MQTT.Message(payload);
   Message.destinationName = "basecamp/sc_sim/cmd";

   if (cmd_id <= 3){
      
       document.getElementById("playback_event_messages").innerHTML = " "
      
       document.getElementById("adcs_mode").value        = '?';
       document.getElementById("adcs_eclipse").value     = '?';

       document.getElementById("cdh_sbc_resets").value   = '?';
       document.getElementById("cdh_hw_cmds").value      = '?';

       document.getElementById("comm_tdrs").value        = '?';
       document.getElementById("comm_data_rate").value   = '?';

       document.getElementById("fsw_rec_plbk").value     = '?';
       document.getElementById("fsw_rec_file_cnt").value = '?';

       document.getElementById("instr_power").value      = '?';
       document.getElementById("instr_science").value    = '?';

       document.getElementById("power_batt_soc").value   = '?';
       document.getElementById("power_sa_curr").value    = '?';

       document.getElementById("therm_htr_1").value      = '?';
       document.getElementById("therm_htr_2").value      = '?';

   }
   
   client.send(Message);
   document.getElementById("messages").innerHTML += "<span> Sent"+payload+" to "+Message.destinationName+"</span><br>";

}

function boolStr(bool){
    str = 'True'
    if (bool == '0') str = 'False'
    return str
}

function eventTypeStr(type){
    str = 'Undefined';
    switch(type) {
    case 1:
       str = 'Debug';
       break;
    case 2:
       str = 'Information';
       break;
    case 3:
       str = 'Error';
       break;
    case 4:
       str = 'Critical';
       break;
    }
    return str;
}