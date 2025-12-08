var restartLocation = window.location.href;

var tickDiff = false;

// system info (platform, product, unit, status)
var devinfo = {};
// other info
var bridgename = "";
var vdchostdsuid = "";
// bridge API status
var bridgeinfo = "";
// plain scenes list (name/no), sorted, for creating selectors
var sceneslist = [];


/* simplistic p44 progress bar */

function setProgressBar(progBar, percentage)
{
  if (percentage<0) percentage = 0;
  else if (percentage>100) percentage = 100;
  percentage = Math.round(percentage);
  var w = progBar.width()/100*percentage;
  if (w<progBar.height()) w = progBar.height();
  progBar.find('.p44_progressbar_gauge').width(w);
  progBar.find('.p44_progressbar_text').html(percentage.toString()+'%');
}


function startTimeProgress(progBar, waitTime)
{
  setProgressBar(progBar, 0);
  var progressStart = (new Date).getTime(); // MS since 1970
  var i = setInterval(function() {
    var nowtime = (new Date).getTime(); // MS since 1970
    var percentage = (nowtime-progressStart)/waitTime*100;
    setProgressBar(progBar, percentage);
    if (percentage>=100) clearInterval(i); // stop
  }, waitTime/100);
}


function dispStr(prop)
{
  return (prop ? prop : "-");
}


function optValSel(optval, selval)
{
  return 'value="' + optval + '"' + (optval==selval ? ' selected="1"' : '');
}


function dsuidShort(dsuid)
{
  return dsuid.substr(0,8) + '...' + dsuid.substr(-2);
}


function opStateSpan(opState, msg, present)
{
  if (opState!=undefined) {
    if (present) {
      var fmt = 'style="color:rgb(' + Math.round(255-Math.max(opState-50,0)*2*2.55).toString() + ', ' + Math.round(Math.min(200,opState*4)).toString() + ', 0)"';
    }
    else {
      var fmt = 'class="notpresent"';
    }
    return '<span ' + fmt + '>' + msg + '</span>';
  }
  return msg;
}


// often used pattern: close dialog, hide loading, on success refresh devices, otherwise show error in alert
function closeDialogAndRefreshDevices(theCall)
{
  theCall.always(function() {
    $.mobile.loading("hide");
  }).done(function() {
    closeDialog(function() {
      refresh_devicelist(false);
    });
  }).fail(showError);
  return theCall;
}


// App Start
// =========

function initializeApp()
{
  // Initialize
  // - get system info
  refresh_sysinfo().always(function() {
    // - get bridge info
    var dfd = refresh_bridgeinfo()

    dfd.promise().always(function() {

      // P44-LC
      refresh_triggerslist(); // need it anyway
      // Default page does not get a 'pageload' event when first shown
      // So explicitly trigger loading/refreshing it here
      $.mobile.loadPage("#lights").done(function() {
        refresh_lightspage();
      });

      // Handlers for Lights page

      // load lights list when page is opened
      $("#lights").on('pageshow', function(event) {
        refresh_lightspage();
      });

      // load current trigger list when page is opened
      $("#triggers").on('pageshow', function(event) {
        refresh_triggerspage();
      });


      // Handlers for Device page

      // load current device list when page is opened
      $("#devices").on('pageshow', function(event) {
        refresh_devicelist();
      });

      $("#devices").on('pagehide', function(event) {
        schedule_refresh_devices_statusinfo(0); // disable refresh
      });

      // refresh DMX dependencies
      $("#dmxCreateDevice").on('popupafteropen.dmxcd', function( event ) {
        $('#dmxCreateDeviceForm').each (function(){
          this.reset();
        });
        refresh_dmx_deps();
      });

      // load DALI dimmers when DALI grouping dialog is opened
      $("#daliCreateGroup").on('popupafteropen.dalicg', function(event) {
        refresh_dali_grouping_deps();
      });

      // refresh customio dependencies on popup open
      $("#customioCreateDevice").on('popupafteropen.ciocd', function( event ) {
        $('#customioCreateDeviceForm').each (function(){
          this.reset();
        });
        refresh_customio_deps();
      });



      // Handlers for System page

      // load latest IP settings and system info when system pane is opened
      $("#system").on('pageshow', function(event) {
        refresh_sysinfo().always(function(){
          refresh_ipconfig();
            // log is integrated on system page
          refresh_vdcd_log();
          });
      });

      install_longclickables()

      // check alerts/notifications
        setTimeout(function() { checkalerts(); }, 2000);
      });
  });
};



// Alerts
// ======

function checkalerts()
{
  p44mCall({
   "cmd":"alert"
  }, 30000).done(function(result) {
    if (result) {
      // there is an alert
      $('#alertDismiss').off("click.adismiss");
      {
        // standard alert
        $('#alertDismiss').html("Dismiss").trigger("create");
        $('#alertDismiss').on("click.adismiss", function(event) {
          dismissalert(result.id)
        });
      }
      // now show alert
      $('#alertTitle').html(result.title);
      $('#alertText').html(result.message);
      openDialog('#alertMessage');
    }
    else {
      // no regular alert
      checkExtraAlerts();
    }
  });
}


function checkExtraAlerts()
{
}




function dismissalert(id, doneCB)
{
  alertError(p44mCall({
    "cmd":"alert",
    "confirm":id
  }, 30000)).done(function(result) {
    closeDialog(doneCB);
  });
}



// Lights Page
// ===========

var currentZoneID = false;
var currentGroupNo = 1; // default to light

function refresh_lightspage()
{
  var dfd = $.Deferred();
  refresh_zones_and_groups().always(function() {
    if (typeof(zonelist[currentZoneID])!="object") {
      // no known zone selected
      for (var zoneid in zonelist) {
        zone = zonelist[zoneid];
        if (zoneid!=0 && zone.deviceCount>0) {
          currentZoneID = zoneid;
          break;
        }
      }
    }
    if (typeof(grouplist[currentGroupNo])!="object") {
      // no known group selected
      for (var groupno in grouplist) {
        group = grouplist[groupno];
        if (groupno!=0) {
          currentGroupNo = groupno;
          break;
        }
      }
    }
    // refresh the zone selector options
    $('#lightZoneSelector').html(zoneselectoroptions(currentZoneID, false)).trigger("create");
    $('#lightGroupSelectorSpace').html(groupselector('lightGroupSelector', currentGroupNo)).trigger("create");
    $('#lightGroupSelector').off("change.lightGroup");
    $('#lightGroupSelector').on("change.lightGroup", function() {
      currentGroupNo = $('#lightGroupSelector').val();
      refresh_lightslist();
      refresh_userSceneslist();
    });

    $('#lightZoneSelector').val(currentZoneID).selectmenu('refresh');
    // refresh the list with devices in currentZone
    refresh_lightslist().always(function() {
      // refresh the list of scenes in currentZone
      refresh_userSceneslist().always(function() {
        dfd.resolve();
      });
    });
  });
  return dfd.promise();
}


function lightZoneSelectorChanged()
{
  currentZoneID = $('#lightZoneSelector').val();
  selectedLight = undefined;
  refresh_lightslist();
  refresh_userSceneslist();
}

var extendedScenes = false;


function refresh_userSceneslist()
{
  var dfd = $.Deferred();
  // query scenes
  var listquery = {
    "method":"x-p44-queryScenes",
    "dSUID":"root",
    "zoneID":currentZoneID,
    "group":currentGroupNo
  };
  if (extendedScenes) {
    listquery.required=0;
  }
  apiCall(
    listquery, 15000
  ).done(function(scenes) {
    // table header
    var tableHtml =
      '<table data-role="table" id="scenesTable" data-mode="columntoggle"'+
      ' class="ui-body-a ui-shadow table-stripe ui-responsive" data-column-btn-theme="a"' +
      ' data-column-btn-text="Columns to display..." data-column-popup-theme="a">' +
      '<thead>' +
       '<tr class="ui-bar-a">' +
         '<th>Scene</th>' +
         '<th data-priority="1">Name</th>' +
         '<th class="actioncell">' +
            '<abbr title="Toggle extended scenes"><a onClick="extendedScenesToggle();" data-inline="true" data-mini="true" data-role="button">' + (extendedScenes ? "Standard" : "Extended") + '</a></abbr>' +
         '</th>' +
       '</tr>' +
      '</thead>' +
      '<tbody>';
    // iterate rows
    for (var sceneKey in scenes) {
      var scene = scenes[sceneKey];
      tableHtml += '<tr>';
      tableHtml += '<td class="desccell">' + scene.action + '</td>';
      tableHtml +=
        '<td class="namecell">' +
        '<abbr title="Rename Scene..."><a onclick="openRenameScene(\'' + scene.id + '\');" data-mini="true" data-role="button" data-icon="edit" data-iconpos="notext" data-theme="a" data-inline="true">Rename</a></abbr> ' +
        scene.name +
        '</td>';
      tableHtml += '<td class="actioncell">';
      tableHtml += '<abbr title="Edit Scene..."><a style="display: ' + (selectedLight ? 'inline-block' : 'none') + ';" class="sceneEditor" onclick="editScene(' + scene.no.toString() + ', selectedLight, ' + "'" + scene.action + (scene.name.length>0 ? ' - ' + scene.name : '') + "'" + ');" data-mini="true" data-role="button" data-icon="edit" data-theme="c" data-inline="true">Edit...</a></abbr>';
      tableHtml += '<abbr title="Call Scene"><a onclick="callScene(' + scene.no.toString() + ',' + currentZoneID.toString() + ',' + currentGroupNo.toString() + ', event);" data-mini="true" data-role="button" data-icon="action" data-theme="a" data-inline="true">Call</a></abbr>';
      tableHtml += '<abbr title="Save current state in this scene..."><a onclick="saveScene(' + scene.no.toString() + ',' + currentZoneID.toString() + ',' + currentGroupNo.toString() + ');" data-mini="true" data-role="button" data-icon="camera" data-theme="a" data-inline="true">Save...</a></abbr>';
      tableHtml += '</td></tr>';
    }
    // finalisation of table
    tableHtml +=
      '</tbody>' +
      '</table>';
    $("#scenesList").html(tableHtml).trigger('create');
    dfd.resolve();
  }).fail(function(domain, code, message) {
    dfd.reject(domain, code, message);
  });
  return dfd.promise();
}


function extendedScenesToggle()
{
  extendedScenes = !extendedScenes;
  refresh_userSceneslist();
}



function openRenameScene(sceneId)
{
  apiCall({
    "method":"getProperty",
    "dSUID":"root",
    "query":{
      "x-p44-localController":{ "scenes": { [sceneId]:{ "name":null } }}
    }
  }).done(function(result) {
    var scene = result["x-p44-localController"].scenes[sceneId];
    var oldname = scene ? scene.name : '';
    $('#newSceneName').val(oldname); // set current name
    $('#sceneNameApply').off("click.sname");
    $('#sceneNameApply').on("click.sname", function(event) { applySceneName(sceneId); });
    openDialog('#scenenameedit');
  });
}


function applySceneName(sceneId)
{
  // get data
  var scenename = $("#newSceneName").val().toString();
  // set new name
  apiCall({
    "method":"setProperty",
    "dSUID":"root",
    "properties":{ "x-p44-localController":{ "scenes": { [sceneId]:{ "name":scenename }}}}
  }).always(function() {
    closeDialog(function() {
      // reload modified scenes list
      refresh_userSceneslist();
    });
  });
}



function callScene(sceneNo, zoneId, groupNo, event)
{
  var target = getTarget(event)
  // call scene
  apiCall({
    "notification":"callScene",
    "zone_id":zoneId,
    "group":groupNo,
    "scene":sceneNo,
    "force":false
  }).always(function() {
    buttonFeedback(target, 'orange');
    setTimeout(function() { refresh_lightslist(); }, 800);
  });
}


function saveScene(sceneNo, zoneId, groupNo)
{
  openDialog('#saveSceneConfirm', function() {
    $("#saveSceneNowButton").off('click.save');
    $("#saveSceneNowButton").on('click.save', function() {
      saveSceneNow(sceneNo, zoneId, groupNo);
    });
  });
}


function saveSceneNow(sceneNo, zoneId, groupNo)
{
  // save scene
  apiCall({
    "notification":"saveScene",
    "zone_id":zoneId,
    "group":groupNo,
    "scene":sceneNo
  }).always(function() {
    closeDialog();
  });
}


var selectedLight = undefined;

function selectDevice(dSUID)
{
  $('.deviceSelector').each(function() {
    $(this).removeClass("ui-btn-c").addClass("ui-btn-a");
  });
  if (dSUID==selectedLight) {
    selectedLight = undefined;
  }
  else {
    selectedLight = dSUID;
    $('#lightSel_'+dSUID).removeClass("ui-btn-a").addClass("ui-btn-c")
  }
  $('.sceneEditor').each(function() {
    if (selectedLight) $(this).show();
    else $(this).hide();
  });
}


function refresh_lightslist()
{
  var dfd = $.Deferred();
  if (currentZoneID==0) {
    // show loader only for whole house, single room should be quick enough
    $.mobile.loading( "show", {
      text: "reloading list...",
      textVisible: true,
      theme: "b"
    });
  }
  // query list
  apiCall({
    "method":"getProperty",
    "dSUID":"root",
    "query":{
      "x-p44-localController":{ "zones":{ [currentZoneID]: { "devices":{ "":
      { "deviceIconName":null, "dSUID":null, "name":null, "model":null, "displayId":null, "x-p44-statusText":null,
        "channelDescriptions":null, "channelStates":null,
        "outputDescription":{"x-p44-behaviourType":null},
        "outputSettings":{"groups":null}
      }}}}}
    }
  }, 15000).done(function(result) {
    // table header
    var tableHtml =
      '<table data-role="table" id="lightsTable" data-mode="columntoggle"'+
      ' class="ui-body-a ui-shadow table-stripe ui-responsive" data-column-btn-theme="a">' +
      '<thead>' +
       '<tr class="ui-bar-a">' +
         '<th class="iconcell" data-priority="3"></th>' +
         '<th class="namecell">Name</th>' +
         '<th class="modelcell" data-priority="3">Model</th>';
    if (currentGroupNo==1) {
      tableHtml +=
         '<th class="brightnesscell">Brightness</th>' +
         '<th class="colorcell" data-priority="1">Color</th>';
    }
    else if (currentGroupNo==2) {
      tableHtml +=
         '<th class="brightnesscell">Position</th>' +
         '<th class="colorcell" data-priority="1">Angle</th>';
    }
    else {
      tableHtml +=
         '<th class="genericcell">Channel 0</th>' +
         '<th class="genericcell" data-priority="1">Channel 1</th>';
    }
    tableHtml +=
         '<th class="actioncell" data-priority="2"></th>' +
       '</tr>' +
      '</thead>' +
      '<tbody>';
    // iterate rows
    var sliders = new Array();
    var devices = undefined;
    var showndevices = 0;
    var zone = result["x-p44-localController"].zones[currentZoneID.toString()];
    if (zone!=undefined) {
      devices = zone.devices;
    }
    if (devices!=undefined) {
      var lightsArray = Object.keys(devices).map(function (key) { return { idx: key, device:devices[key] }; });
      lightsArray = lightsArray.sort(function (a,b) { return a.device.name.localeCompare(b.device.name); });
      for (var i=0; i<lightsArray.length; i++) {
        var device = lightsArray[i].device;
        // only devices in selected group
        if (
          device.outputDescription!=undefined &&
          device.outputSettings.groups[currentGroupNo] // must be true, not falsish (false or undefined)
        ) {
          tableHtml += '<tr>' +
            '<td class="iconcell"><img src="/icons/icon16/' + device.deviceIconName + '.png" /></td>' +
            '<td class="namecell">' + device.name + '</td>' +
            '<td class="modelcell">' + device.model + '</td>';
          if (device.outputDescription['x-p44-behaviourType']=='light') {
            // light behaviour
            tableHtml +=
              '<td class="brightnesscell slidercell"><input type="range" class="slider_brightness" data-highlight="false" name="output_slider_brightness" id="output_slider_brightness_' + device.dSUID + '" min="0" max="100" step="1" value="' + roundToFracDigits(device.channelStates.brightness.value,1) + '" /></td>';
            sliders.push({dSUID:device.dSUID, channelID:'brightness'});
            if (device.channelDescriptions.hue!=undefined) {
              // RGB
              tableHtml += '<td class="colorcell slidercell"><input type="range" class="slider_hue" data-highlight="false" name="output_slider_hue" id="output_slider_hue_' + device.dSUID + '" min="0" max="360" step="1" value="' + roundToFracDigits(device.channelStates.hue.value, 1) + '" />';
              tableHtml += '<input type="range" class="slider_generic" data-highlight="true" name="output_slider_saturation" id="output_slider_saturation_' + device.dSUID + '" min="0" max="100" step="1" value="' + roundToFracDigits(device.channelStates.saturation.value, 1) + '" /></td>';
              sliders.push({dSUID:device.dSUID, channelID:'hue'});
              sliders.push({dSUID:device.dSUID, channelID:'saturation'});
            }
            else if (device.channelDescriptions.colortemp!=undefined) {
              // tunable white
              tableHtml += '<td class="colorcell slidercell"><input type="range" class="slider_colortemp" data-highlight="false" name="output_slider_colortemp" id="output_slider_colortemp_' + device.dSUID + '" min="100" max="1000" step="1" value="' + roundToFracDigits(device.channelStates.colortemp.value,1) + '" /></td>';
              sliders.push({dSUID:device.dSUID, channelID:'colortemp'});
            }
            else {
              // monochrome
              tableHtml += '<td class="colorcell slidercell">&nbsp;</td>';
            }
          }
          else if (currentGroupNo==2 && device.outputDescription['x-p44-behaviourType']=='shadow') {
            // shadow behaviour
            tableHtml += '<td class="slidercell"><input type="range" class="slider_covering" data-highlight="true" name="output_slider_shadePositionOutside" id="output_slider_shadePositionOutside_' + device.dSUID + '" min="0" max="100" step="0.5" value="' + roundToFracDigits(device.channelStates.shadePositionOutside.value, 2) + '" /></td>';
            sliders.push({dSUID:device.dSUID, channelID:'shadePositionOutside'});
            tableHtml += '<td class="slidercell"><input type="range" class="slider_generic" data-highlight="false" name="output_slider_shadeOpeningAngleOutside" id="output_slider_shadeOpeningAngleOutside_' + device.dSUID + '" min="0" max="100" step="1" value="' + roundToFracDigits(device.channelStates.shadeOpeningAngleOutside.value, 0) + '" /></td>';
            sliders.push({dSUID:device.dSUID, channelID:'shadeOpeningAngleOutside'});
          }
          else {
            // generic output, just show sliders for first two channels
            var numchannels = 0
            for (var channelID in device.channelDescriptions) {
              if (++numchannels>2) break;
              var channel = device.channelDescriptions[channelID]
              var channelType = channel.channelType
              var max = channel.max
              var min = channel.min
              var resolution = channel.resolution
              var step = resolution>=(max-min)/200  ? resolution : 0.5 // we cannot have more than 200 steps
              var value = roundToResolution(device.channelStates[channelID].value, resolution)
              tableHtml +=
                '<td class="slidercell"><input title="' + escapehtml(channel.name) + '" class="slider_generic" type="range" data-highlight="false" name="output_slider_generic" id="output_slider_' + channelID + '_' + device.dSUID + '" min="' + min.toString() + '" max="' + max.toString() + '" step="' + step.toString() + '" value="' + value.toString() + '" /></td>';
              sliders.push({dSUID:device.dSUID, channelID:channelID});
            }
            while(numchannels++<2) {
              tableHtml += '<td>&nbsp;</td>';
            }
          }
          tableHtml += '<td class="actioncell">'
          tableHtml += '<abbr title="Select device for scene editing..."><a id="lightSel_' + device.dSUID +'" onclick="selectDevice(\'' + device.dSUID + '\');" class="deviceSelector" data-mini="true" data-role="button" data-icon="star" data-iconpos="notext" data-theme="' + (device.dSUID==selectedLight ? "c" : "a") + '" data-inline="true">Select</a></abbr>';
          tableHtml += '<abbr title="Show all channels..."><a dSUID="' + device.dSUID + '" class="p44-longclickable" p44-handler="openDeviceChannels" data-mini="true" data-role="button" data-icon="gear" data-iconpos="notext" data-theme="a" data-inline="true">Outputs</a></abbr>';
          tableHtml += '<abbr title="Stop Scene Actions..."><a onclick="stopSceneActionsAndRefreshLights(\'' + device.dSUID + '\', event);" data-mini="true" data-role="button" data-icon="forbidden" data-iconpos="notext" data-theme="a" data-inline="true">Stop Scene Actions</a></abbr>';
          tableHtml += '</td></tr>';
          showndevices++;
        }
      }
      if (showndevices==0) {
        tableHtml += '<tr><td colspan="6" class="fullrowinfocell"><span class="warningtext">- nothing in this zone - select different zone from popup above -</span></td></tr>';
      }
    }
    else {
      tableHtml += '<tr><td colspan="6" class="fullrowinfocell"><span class="warningtext">- no zones - connect some lights first -</span></td></tr>';
    }
    // finalisation of table
    tableHtml +=
      '</tbody>' +
      '</table>';
    $("#lightsList").html(tableHtml).trigger('create');
    install_longclickables("#lightsList");
    for (var i in sliders) {
      var slider = sliders[i];
      outputSliderChangeAttachEvent(slider.dSUID, slider.channelID);
    }
    $.mobile.loading( "hide" );
    dfd.resolve();
  }).fail(function(domain, code, message) {
    dfd.reject(domain, code, message);
  });
  return dfd.promise();
}


function outputSliderChangeAttachEvent(dSUID, channelID)
{
  $('#output_slider_' + channelID + '_' + dSUID).off("change.osl");
  $('#output_slider_' + channelID + '_' + dSUID).on("change.osl", function(event, ui) { changedOutputSlider(event, dSUID, channelID)});
}


var outputValueChanging = false;
var outputValueChangePending = false;

function changedOutputSlider(event, dSUID, channelID)
{
  if (outputValueChanging) {
    outputValueChangePending = function() { changedOutputSlider(event, dSUID, channelID); };
  }
  else {
    outputValueChanging = true;
    var value = $('#output_slider_' + channelID + '_' + dSUID).val();
    //console.log('channelID=' + channelID + ' value=' + value.toString());
    var changequery = {
      "method":"setProperty",
      "dSUID":dSUID,
      "properties":{
        "channelStates": { }
      }
    };
    changequery.properties.channelStates[channelID.toString()] = { "value": value };
    apiCall(changequery, 1000).done(function() {
      outputValueChanging = false;
      if (outputValueChangePending) {
        var cb = outputValueChangePending;
        outputValueChangePending = false;
        cb();
      }
    }).fail(function() {
      outputValueChanging = false;
      outputValueChangePending = false;
    });
  }
}


// Triggers Page
// =============


function refresh_triggerspage()
{
  return refresh_triggerslist();
}


function refresh_triggerslist()
{
  var dfd = $.Deferred();
  // query triggers
  apiCall({
    "method":"getProperty",
    "dSUID":"root",
    "query":{
      "x-p44-localController":{ "triggers":null }
    }
  }, 15000).done(function(result) {
    // table header
    var tableHtml =
      '<table data-role="table" id="triggersTable" data-mode="columntoggle"'+
      ' class="ui-body-a ui-shadow table-stripe ui-responsive" data-column-btn-theme="a">' +
      '<thead>' +
       '<tr class="ui-bar-a">' +
         '<th class="namecell">Name</th>' +
         '<th class="triggerconditioncell" data-priority="1">Condition</th>' +
         '<th class="triggeractioncell" data-priority="3">Action</th>' +
         '<th class="triggeruicell" data-priority="2">&nbsp;</th>' +
         '<th class="actioncell"><abbr title="New trigger..."><a class="p44-longclickable" p44-handler="doNewTrigger" data-mini="true" data-role="button" data-icon="plus" data-theme="a" data-inline="true">New...</a></abbr></th>' +
       '</tr>' +
      '</thead>' +
      '<tbody>';
    // create and sort as array
    var triggers = result["x-p44-localController"].triggers;
    var triggersArray = Object.keys(triggers).map(function (key) {
      var tr = triggers[key];
      // parse uiparams field as JSON and replace original text with JSON
      var uiparams = {};
      try {
        uiparams = JSON.parse(tr.uiparams);
      }
      catch {}
      tr.uiparams = uiparams;
      return { key: key, trigger:tr };
    });
    // sort
    triggersArray = triggersArray.sort(function (a,b) {
      var ao = a.trigger.uiparams.order;
      var bo = b.trigger.uiparams.order;
      if (ao) ao = parseInt(ao); else ao = 9999999;
      if (bo) bo = parseInt(bo); else bo = 9999999;
      if (ao==bo)
        return a.trigger.name.localeCompare(b.trigger.name);
      return ao<bo ? -1 : 1;
    });
    // render
    for (var i=0; i<triggersArray.length; i++) {
      var t = triggersArray[i];
      tableHtml += '<tr>';
      tableHtml += '<td class="namecell">' + t.trigger.name + '</td>';
      tableHtml += '<td class="triggerconditioncell">' + t.trigger.condition + '</td>';
      tableHtml += '<td class="triggeractioncell">' + t.trigger.action + '</td>';
      tableHtml += '<td class="triggeruicell"' +
        (t.trigger.uiparams.show ? ' onclick="executeTrigger(' + t.key.toString() + ', event);" style="background-color:' + (t.trigger.uiparams.color ? t.trigger.uiparams.color : '#2ad') + '"' : '') + '>' +
        (t.trigger.uiparams.order && t.trigger.uiparams.show ? t.trigger.uiparams.order.toString() : '&nbsp;') + '</td>';
      tableHtml += '<td class="actioncell">';
      tableHtml += '<abbr title="Edit trigger..."><a onclick="editTrigger(' + t.key.toString() + ');" data-mini="true" data-role="button" data-icon="edit" data-iconpos="notext" data-theme="a" data-inline="true">Edit...</a></abbr>';
      tableHtml += '<abbr title="Run trigger action script now"><a onclick="executeTrigger(' + t.key.toString() + ', event);" data-mini="true" data-role="button" data-icon="action" data-iconpos="notext" data-theme="a" data-inline="true">Run</a></abbr>';
      tableHtml += '<abbr title="Stop running trigger script"><a onclick="stopTrigger(' + t.key.toString() + ', event);" data-mini="true" data-role="button" data-icon="forbidden" data-theme="a" data-iconpos="notext" data-inline="true">Stop</a></abbr>';
      tableHtml += '</td></tr>';
    }
    // finalisation of table
    tableHtml +=
      '</tbody>' +
      '</table>';
    $("#triggersList").html(tableHtml).trigger('create');
    install_longclickables("#triggersList");
    dfd.resolve();
  }).fail(function(domain, code, message) {
    dfd.reject(domain, code, message);
  });
  return dfd.promise();
}


function doNewTrigger(event, hiddenFeatures)
{
  if (hiddenFeatures==2) {
    if (event.shiftKey || hiddenFeatures==1) {
      // show REPL in separate page
      openREPL()
      return
    }
    // hidden direct link to main script editing
    openMainScript()
    return
  }
  else if (hiddenFeatures==1) {
    openDevTabs()
    return
  }
  newTrigger()
}


function newTrigger() {
  apiCall({
    "method":"setProperty",
    "dSUID":"root",
    "properties":{
      "x-p44-localController":{ "triggers": { "0":{ "name":"New Trigger" } } }
    }
  }).done(function(result) {
    var triggerID = result[0].element;
    editTrigger(triggerID);
  });
}


function set_tr_logoffs(triggerId, offs)
{
  apiCall({
    "method":"setProperty",
    "dSUID":"root",
    "properties":{
      "x-p44-localController":{ "triggers": { [triggerId]:{ "logLevelOffset":offs } } }
    }
  });
}


function refresh_showonactionpage_deps()
{
  var showaction = $("#showOnActionPageSelect").val();
  showIf(showaction!=0, '#actionPageRenderingOptions');
  setTimeout(function() { $("#triggerEdit").popup("reposition", {positionTo: 'window'}); }, 1 );
}



function editTrigger(triggerId) {
  varDefsHandler.loadSources(true).done(function() {
    triggerId = triggerId.toString();
    // query trigger details
    apiCall({
      "method":"getProperty",
      "dSUID":"root",
      "query":{
        "x-p44-localController":{ "triggers": { [triggerId]:null } }
      }
    }).done(function(result) {
      var trigger = result["x-p44-localController"].triggers[triggerId];
      // var defs
      varDefsHandler.setVarDefs(trigger['varDefs']);
      varDefsHandler.renderVarDefs('trigger');
      // action page params
      var uiparams = false;
      try {
        uiparams = JSON.parse(trigger.uiparams);
      } catch (err) {}
      if (!uiparams) uiparams = { "show":false };
      // reset checks
      $('#triggerVarDefsCheck').html('');
      $('#triggerConditionCheck').html('');
      $('#triggerActionCheck').html('');
      // fields
      $('#triggerNameTextfield').val(trigger.name);
      $('#triggerConditionTextfield').val(trigger.condition);
      $('#triggerModeSelect').val(trigger.mode).selectmenu("refresh");
      $('#triggerActionTextfield').val(trigger.action);
      // uiparams
      $('#showOnActionPageSelect').val(uiparams.show ? 1 : 0).slider("refresh");
      // - subfields
      $('#actionSectionTitleTextField').val(uiparams.sectiontitle);
      $('#actionNameTextField').val(uiparams.text);
      $('#actionColorTextField').val(uiparams.color);
      $('#actionOrderTextField').val(uiparams.order);
      $('#actionTypeSelect').val(uiparams.type ? uiparams.type : 'button').selectmenu("refresh");
      activateIDElink('#triggerActionScriptEditorLink', trigger.actionId, function() { return saveTriggerData(triggerId, true) });
      // set apply handlers
      $("#triggerEditApply").off('click.edit'); // remove previous one
      $("#triggerEditApply").on('click.edit', function () { saveTrigger(triggerId, false, false); });
      $("#triggerConditionTestButton").off('click.edit'); // remove previous one
      $("#triggerConditionTestButton").on('click.edit', function () { saveTrigger(triggerId, true, false, event); });
      $("#triggerActionTestButton").off('click.edit'); // remove previous one
      $("#triggerActionTestButton").on('click.edit', function () { saveTrigger(triggerId, false, true, event); });
      $("#triggerActionStopButton").off('click.edit'); // remove previous one
      $("#triggerActionStopButton").on('click.edit', function () { stopTrigger(triggerId, event); });
      $("#triggerEditRemove").off('click.edit'); // remove previous one
      $("#triggerEditRemove").on('click.edit', function () { deleteTrigger(triggerId); });
      // log offset controls
      $("#triggerLogOffset").html(
        '<p><b>Logging:</b>&nbsp;<a title="min logging" href="javascript:set_tr_logoffs(\'' + triggerId + '\',-2);">-2</a>&nbsp;' +
        '<a title="less logging" href="javascript:set_tr_logoffs(\'' + triggerId + '\',-1);">-1</a>&nbsp;' +
        '<a title="default logging" href="javascript:set_tr_logoffs(\'' + triggerId + '\',0);">normal</a>&nbsp;' +
        '<a title="more logging" href="javascript:set_tr_logoffs(\'' + triggerId + '\',1);">+1</a>&nbsp;' +
        '<a title="max logging" href="javascript:set_tr_logoffs(\'' + triggerId + '\',2);">+2</a></p>'
      );
      refresh_showonactionpage_deps()
      openDialog('#triggerEdit');
    });
  });
}


function saveTriggerData(triggerId, emptyAsSTX)
{
  var dfd = $.Deferred()
  var src = $("#triggerActionTextfield").val()
  if (src.length==0 && emptyAsSTX) src = "\x02";
  // create uiparams JSON text
  var uiparams = {}
  uiparams.show = $("#showOnActionPageSelect").val()>0
  if ($('#actionSectionTitleTextField').val().length>0) uiparams.sectiontitle = $('#actionSectionTitleTextField').val()
  if ($('#actionNameTextField').val().length>0) uiparams.text = $('#actionNameTextField').val()
  if ($('#actionColorTextField').val().length>0) uiparams.color = $('#actionColorTextField').val()
  if ($('#actionOrderTextField').val().length>0) uiparams.order = $('#actionOrderTextField').val()
  if ($('#actionTypeSelect').val().length>0) uiparams.type = $('#actionTypeSelect').val()
  apiCall({
    method:"setProperty",
    dSUID:"root",
    properties:{
      "x-p44-localController":{ triggers: { [triggerId]:{
        name:$("#triggerNameTextfield").val(),
        condition:$("#triggerConditionTextfield").val(),
        action:src,
        mode:$("#triggerModeSelect").val(),
        uiparams:JSON.stringify(uiparams),
        varDefs:varDefsHandler.vardefs
      }}}
    }
  }).done(function() {
    // things depending on saved trigger can start now
    dfd.resolve()
  })
  return dfd
}


function saveTrigger(triggerId, checkCondition, checkAction, event)
{
  var target = getTarget(event)
  saveTriggerData(triggerId).done(function() {
    // things depending on saved trigger can start now
    // immediate feedback on button
    buttonFeedback(target, 'green');
    // check or just close?
    if (checkCondition || checkAction) {
      if (checkCondition) {
        apiCall({
          "method": "x-p44-checkTriggerCondition",
          "dSUID":"root",
          "triggerID":triggerId
        }).done(function(result) {
          $('#triggerVarDefsCheck').html(varDefsHandler.varShowHtml(result.varDefs));
          var cond = result.condition;
          if (cond.error) {
            condhtml = '<span class="evaluationCheckError">' + escapehtml(cond.error) + '</span>';
            if (cond.at!==undefined) selectText($('#triggerConditionTextfield'), cond.at, cond.at+1)
          }
          else if (cond.result==null) {
            condhtml = '<span class="evaluationCheckNull">' + escapehtml(cond.text) + '</span>'
          }
          else {
            condhtml = 'Condition evaluates to <b>' + escapehtml(cond.text) + '</b> (' + (cond.result>0 ? 'true' : 'false') + ')'
          }
          $('#triggerConditionCheck').html(condhtml).trigger('create')
        });
      }
      if (checkAction) {
        apiCall({
          "method": "x-p44-testTriggerAction",
          "dSUID":"root",
          "triggerID":triggerId
        }).done(function(cond) {
          if (cond.error) {
            condhtml = '<span class="evaluationCheckError">' + escapehtml(cond.error) + '</span>'
            if (cond.at!==undefined) selectText($('#triggerActionTextfield'), cond.at, cond.at+1)
          }
          else {
            condhtml = 'Successfully executed'
            if (cond.result!=undefined) condhtml += ' (script return value: <b>' + escapehtml(cond.result.toString()) + '</b>)'
          }
          $('#triggerActionCheck').html(condhtml).trigger('create')
        });
      }
    }
    else {
      closeDialog(function() {
        // reload modified trigger list
        refresh_triggerslist()
      })
    }
  })
}



function deleteTrigger(triggerId)
{
  openDialog("#removeTriggerConfirm", function() {
    $("#removeTriggerNowButton").off('click.edit'); // remove previous one
    $("#removeTriggerNowButton").on('click.edit', function () { deleteTriggerNow(triggerId); });
  });
}


function deleteTriggerNow(triggerId)
{
  // remove now
  apiCall({
    "method":"setProperty",
    "dSUID":"root",
    "properties":{
      "x-p44-localController":{ "triggers": { [triggerId]:null }}
    }
  }).always(function() {
    // reload modified device list
    closeDialog(function () {
      refresh_triggerslist();
    });
  });
}


function executeTrigger(triggerId, event)
{
  var target = getTarget(event)
  // execute
  apiCall({
    "method": "x-p44-testTriggerAction",
    "dSUID":"root",
    "triggerID":triggerId
  }).done(function() {
    buttonFeedback(target, 'green')
  });
}


function stopTrigger(triggerId, event)
{
  var target = getTarget(event)
  apiCall({
    "method": "x-p44-stopTriggerAction",
    "dSUID":"root",
    "triggerID":triggerId
  }).done(function() {
    buttonFeedback(target, 'red')
  });
}






function editScene(sceneNo, dSUID, title)
{
  // query scene details
  apiCall({
    "method":"getProperty",
    "dSUID":dSUID,
    "query":{
      "name":null,
      "scenes":{ [sceneNo.toString()] : null }
    }
  }).done(function(result) {
    var scene = result.scenes[sceneNo.toString()]
    // fields
    $('#sceneEditorTitle').html('Scene: "' + escapehtml(title) + '" in device "' + (result.name.length>0 ? result.name : dSUID) + '"')
    $('#sceneDontcareSelect').val(scene.dontCare ? 1 : 0).slider("refresh")
    $('#sceneIgnorePrioSelect').val(scene.ignoreLocalPriority ? 1 : 0).slider("refresh")
    $('#sceneEffectSelect').val(scene.effect).selectmenu("refresh")
    $('#sceneEffectParamEdit').val(scene.effectParam)
    $('#sceneScriptTextfield').val(scene['x-p44-sceneScript'])
    activateIDElink(
      '#sceneScriptEditorLink',
      function() { // ID getter
        var dfd = $.Deferred();
        var scriptid = scene['x-p44-sceneScriptId']
        if (scriptid) {
          // we have it already
          dfd.resolve(scriptid)
        }
        else {
          // we need to get it because the script was not active when the dialog was loaded
          apiCall({
            "method":"getProperty",
            "dSUID":dSUID,
            "query":{
              "name":null,
              "scenes":{ [sceneNo.toString()] : { 'x-p44-sceneScriptId': null } }
            }
          }).done(function(result) {
            dfd.resolve(result.scenes[sceneNo.toString()]['x-p44-sceneScriptId'])
          })
        }
        return dfd
      },
      function() { // script saver
        return saveSceneData(sceneNo, dSUID, true)
      }
    )
    // set apply handlers
    $("#sceneEditApply").off('click.edit'); // remove previous one
    $("#sceneEditApply").on('click.edit', function () { saveSceneEdits(sceneNo, dSUID, false); });
    $("#sceneTestButton").off('click.edit'); // remove previous one
    $("#sceneTestButton").on('click.edit', function () { saveSceneEdits(sceneNo, dSUID, true, event); });
    $("#sceneStopButton").off('click.edit'); // remove previous one
    $("#sceneStopButton").on('click.edit', function () { stopSceneActions(dSUID, event); });
    openDialog('#sceneEditor');
    refresh_sceneeffect_deps();
  });
}


function refresh_sceneeffect_deps()
{
  // current settings
  var effect = $("#sceneEffectSelect").val();

  // basic type
  showIf(effect==6, '#sceneEditorScriptEffect');
  showIf(effect==4 || effect==5, '#sceneEditorEffectParam');
  setTimeout(function() { $("#sceneEditor").popup("reposition", {positionTo: 'window'}); }, 1 );
}


function saveSceneData(sceneNo, dSUID, emptyAsSTX)
{
  var dfd = $.Deferred()
  // save changes
  var src = $('#sceneScriptTextfield').val()
  if (src.length==0 && emptyAsSTX) src = "\x02";
  apiCall({
    "method":"setProperty",
    "dSUID":dSUID,
    "properties":{
      "scenes":{ [sceneNo.toString()] : {
        "dontCare": $('#sceneDontcareSelect').val()>0 ? true : false,
        "ignoreLocalPriority": $('#sceneIgnorePrioSelect').val()>0 ? true : false,
        "effect": $('#sceneEffectSelect').val(),
        "effectParam": $('#sceneEffectParamEdit').val(),
        "x-p44-sceneScript": src
      }}
    }
  }).done(function() {
    dfd.resolve()
  })
  return dfd
}


function saveSceneEdits(sceneNo, dSUID, checkCall, event)
{
  var target = getTarget(event)
  // save changes
  saveSceneData(sceneNo, dSUID).done(function() {
    buttonFeedback(target, 'green')
    // check or just close?
    if (checkCall) {
      // call scene
      apiCall({
        "notification":"callScene",
        "dSUID":dSUID,
        "scene":sceneNo,
        "force":false
      }).done(function(cond) {
        // NOP
      });
    }
    else {
      closeDialog(function() {
        // reload modified trigger list
        refresh_userSceneslist();
      });
    }
  });
}




// Device Page
// ===========

var activeRequest = 0;

function learn(noProximity)
{
  openDialog("#learnWait");
  show('#learnRequest');
  show('#learnCancel');
  hide('#learnResult');
  hide('#learnOK');
  var resultHtml = '';
  activeRequest = apiCall({
    "method":"x-p44-learn",
    "dSUID":"root",
    "seconds":30,
    "disableProximityCheck":noProximity
  }, 120000).done(function(learnResult) {
    if (learnResult==true)
      resultHtml = '<p style="color: green; font-weight: bold;">Learned in device successfully.</p>';
    else
      resultHtml = '<p style="color: red; font-weight: bold;">Learned out device successfully.</p>';
  }).fail(function(domain, code, message) {
    if (domain=="p44vdc" && code==408) {
      // timed out,
      resultHtml = "<p>Timeout: No learn event received</p>";
    }
    else {
      resultHtml = "<p>API Error</p>";
      console.log('API error ' + code + ' ' + message);
    }

  }).always(function() {
    // new info
    hide('#learnRequest');
    hide('#learnCancel');
    $('#learnResult').html(resultHtml);
    show('#learnResult');
    show('#learnOK');
    setTimeout(function() { $("#learnWait").popup("reposition", {positionTo: 'window'}); }, 1 );
    // redisplay device list
    refresh_devicelist();
  });
}


function stopLearn()
{
  closeDialog();
  activeRequest.abort();
  activeRequest = undefined;
  apiCall({
    "method":"x-p44-learn",
    "dSUID":"root",
    "seconds":0
  });
}


function identify()
{
  openDialog("#identifyWait");
  show('#identifyRequest');
  show('#identifyCancel');
  hide('#identifyResult');
  hide('#identifyOK');
  var resultHtml = '';
  var focusDSUID = false;
  activeRequest = apiCall({
    "method":"x-p44-identify",
    "dSUID":"root",
    "seconds":30
  }, 120000).done(function(identifiedDsuid) {
    focusDSUID = identifiedDsuid;
    if (focusDSUID!=false) {
      resultHtml = '<p style="color: green; font-weight: bold;">Identified Device ' + focusDSUID + '</p>';
    }
  }).fail(function(domain, code, message) {
    if (domain=="p44vdc" && code==408) {
      // timed out,
      resultHtml = "<p>Timeout: No device was identified</p>";
    }
    else {
      resultHtml = "<p>API Error</p>";
      console.log('API error ' + code + ' ' + message);
    }
  }).always(function() {
    // new info
    hide('#identifyRequest');
    hide('#identifyCancel');
    $('#identifyResult').html(resultHtml);
    show('#identifyResult');
    show('#identifyOK');
    setTimeout(function() { $("#identifyWait").popup("reposition", {positionTo: 'window'}); }, 1 );
    // redisplay device list with focus set
    refresh_devicelist(focusDSUID);
  });
}


function stopIdentify()
{
  closeDialog();
  activeRequest.abort();
  activeRequest = undefined;
  apiCall({
    "method":"x-p44-identify",
    "dSUID":"root",
    "seconds":0
  });
}




function hueBridgeOptions(dSUID)
{
  openDialog("#hueBridgeOptions");
  $("#saveHueApiUrlButton").off('click.saveApi');
  $("#saveHueApiUrlButton").on('click.saveApi', function() {
    // set hue bridge API URL
    var apiLoc = $("#hueBridgeLoc").val().trim();
    closeDialogAndRefreshDevices(apiCall({
      "method":"registerHueBridge",
      "dSUID":dSUID,
      "bridgeApiURL":apiLoc
    }));
  });
}




function refresh_customio_deps()
{
  // current settings
  var custom_devicetype = $("#custom_devicetype_select").val();
  var digitalio_device = $("#digitalio_device_select").val();
  var analogio_device = $("#analogio_device_select").val();
  // dependent settings
  var show_digitalio = false;
  var show_analogio = false;
  var show_consoleio = false;
  var show_custom = false;
  var show_gpio_params = false;
  var show_led_params = false;
  var show_i2c_params = false;
  var show_spi_params = false;
  var show_pwm_params = false;

  // basic type
  switch (custom_devicetype) {
    case "digitalio" : show_digitalio = true; break;
    case "analogio" : show_analogio = true; break;
    case "console" : show_consoleio = true; break;
    case "custom" : show_custom = true;
  }
  showIf(show_digitalio, '.digitalio_group');
  showIf(show_analogio, '.analogio_group');
  showIf(show_consoleio, '.consoleio_group');
  showIf(show_custom, '.custom_group');
  // physical I/O selectors
  show_gpio_params = (show_digitalio && digitalio_device=="gpio");
  show_led_params = (show_digitalio && digitalio_device=="led");
  show_i2c_params = (show_digitalio && digitalio_device=="i2c") || (show_analogio && analogio_device=="i2c");
  show_spi_params = (show_digitalio && digitalio_device=="spi") || (show_analogio && analogio_device=="spi");
  show_pwm_params = (show_analogio && analogio_device=="pwm");
  showIf(show_gpio_params, '.gpio_group');
  showIf(show_led_params, '.led_group');
  showIf(show_i2c_params, '.i2c_group');
  showIf(show_spi_params, '.spi_group');
  showIf(show_pwm_params, '.pwm_group');
  setTimeout(function() { $("#customioCreateDevice").popup("reposition", {positionTo: 'window'}); }, 1 );
}


function createCustomIODevice()
{
  // create custom I/O device
  // - current settings
  var custom_devicetype = $("#custom_devicetype_select").val();
  var devtype = custom_devicetype; // default to selected type
  var iospec = ""; // none
  if (custom_devicetype=="custom") {
    // manually entered
    devtype = $("#custom_devicetype_textfield").val();
    iospec = $("#custom_devicespec_textfield").val();
  }
  else if (custom_devicetype=="console") {
    iospec =  $("#customdev_name_textfield").val() + ":" + $("#consoleio_behaviour_select").val();
  }
  else {
    // create iospec from selections
    var behaviour = "";
    var devicekind = "missing";
    var i2cchip = "generic";
    var spichip = "generic";
    if (custom_devicetype=="digitalio") {
      behaviour = $("#digitalio_behaviour_select").val();
      devicekind = $("#digitalio_device_select").val();
      i2cchip = $("#digitalio_i2c_chip_select").val();
      spichip = $("#digitalio_spi_chip_select").val();
    }
    else if (custom_devicetype=="analogio") {
      behaviour = $("#analogio_behaviour_select").val();
      devicekind = $("#analogio_device_select").val();
      i2cchip = $("#analogio_i2c_chip_select").val();
      spichip = $("#analogio_spi_chip_select").val();
    }
    var devicespec = devicekind; // default to just device kind
    var pinNo = false;
    var suffix = false;
    if (devicekind=="gpio") {
      pinNo = $("#gpio_pin_textfield").val();
    }
    else if (devicekind=="led") {
      pinNo = $("#led_pin_textfield").val();
    }
    else if (devicekind=="i2c") {
      devicespec += $("#i2c_bus_select").val() + "." + i2cchip; // add bus and chip driver name
      devicespec += "@" + $("#i2c_address_textfield").val(); // add i2c address
      pinNo = $("#i2c_pin_textfield").val();
    }
    else if (devicekind=="spi") {
      devicespec += $("#spi_bus_select").val() + "." + spichip; // add bus and chip driver name
      devicespec += "@" + $("#spi_address_textfield").val(); // add SPI address
      pinNo = $("#spi_pin_textfield").val();
    }
    else if (devicekind=="pwm") {
      devicespec += "chip" + $("#pwm_chip_textfield").val();
      pinNo = $("#pwm_channel_textfield").val(); // channel is the "pinNo" for PWMs
      suffix = "." + $("#pwm_period_textfield").val(); // add period as suffix
    }
    // prefix with pin options, if any
    if ($("#digitalio_pinopt_select").val()!="X") {
      devicespec = $("#digitalio_pinopt_select").val().toString() + devicespec.toString();
    }
    // use consecutive outputs for CWWW, RGB, RGBW
    if (behaviour=="cwwwdimmer") {
      // CWWW: two consecutive pins
      var i = parseInt(pinNo); // number
      iospec =
        devicespec + "." + i.toString() + (suffix ? suffix : "") + "|" +
        devicespec + "." + (i+1).toString() + (suffix ? suffix : "");
      iospec += ":" + behaviour;
    }
    else if (behaviour=="rgbdimmer") {
      // RGB: three consecutive pins
      var i = parseInt(pinNo); // number
      iospec =
        devicespec + "." + i.toString() + (suffix ? suffix : "") + "|" +
        devicespec + "." + (i+1).toString() + (suffix ? suffix : "") + "|" +
        devicespec + "." + (i+2).toString() + (suffix ? suffix : "");
      iospec += ":" + behaviour;
    }
    else if (behaviour=="rgbwdimmer") {
      // RGBW: use four consecutive pins
      var i = parseInt(pinNo); // number
      iospec =
        devicespec + "." + i.toString() + (suffix ? suffix : "") + "|" +
        devicespec + "." + (i+1).toString() + (suffix ? suffix : "") + "|" +
        devicespec + "." + (i+2).toString() + (suffix ? suffix : "") + "|" +
        devicespec + "." + (i+3).toString() + (suffix ? suffix : "");
      iospec += ":" + "rgbdimmer"; // presence of white determines RGBW functionality
    }
    else {
      // single pin
      if (pinNo !== false) devicespec = devicespec.toString() + (suffix ? suffix : "") + "." + pinNo.toString();
      iospec = devicespec + ":" + behaviour;
    }
  }
  //alert("devtype = " + devtype + "\niospec = " + iospec);
  var deviceName = $("#customdev_name_textfield").val();
  closeDialogAndRefreshDevices(apiCall({
    "method":"x-p44-addDevice",
    "dSUID":"none", "x-p44-itemSpec":"vdc:Static_Device_Container:1",
    "deviceType": devtype,
    "deviceConfig": iospec,
    "name" : deviceName
  }));
}


function refresh_rgbchain_autosize_deps()
{
  showIf($("#rgbchain_autosize").val()==0, '#rgbchain_manualsize');
}


function createRGBLedChainDevice()
{
  // create RGB LED chain device
  if (
    $("#rgbchain_type_select").val()=='' ||
    ($("#rgbchain_autosize").val()==0 && ($("#rgbchain_x_textfield").val()=='' || $("#rgbchain_dx_textfield").val()==''))
  ) {
    alert("Please specify at least type and (automatic or specific) size of light");
    return;
  }
  if ($("#rgbchaindev_uniqueid_textfield").val().includes(':')) {
    alert("Unique ID must not contain interpunction or special characters (just leave it empty to autogenerate ID)");
    return;
  }
  var deviceconfig = $("#rgbchain_type_select").val();
  var uniqueId = $("#rgbchaindev_uniqueid_textfield").val().trim();
  if (uniqueId.length==0) {
    // automatic ID
    uniqueId = (16+Math.floor(Math.random() * 239)).toString(16)+"-"+Date.now().toString(16);
  }
  // dx==dy==0 means autosizing
  var x = 0;
  var dx = 0;
  var y = 0;
  var dy = 0;
  if ($("#rgbchain_autosize").val()==0) {
    // manually specified
    x = parseInt($("#rgbchain_x_textfield").val());
    dx = parseInt($("#rgbchain_dx_textfield").val());
    y = parseInt($("#rgbchain_y_textfield").val()); if (Number.isNaN(y)) y=0;
    dy = parseInt($("#rgbchain_dy_textfield").val()); if (Number.isNaN(dy)) y=1;
    if (x<0 || dx<1 || y<0 || dy<1) {
      // need some config
      alert("Please specify non-zero Light size or choose automatic sizing");
      return;
    }
  }
  var z_order = parseInt($("#rgbchain_zorder_textfield").val());
  var deviceName = $("#rgbchaindev_name_textfield").val();
  var deviceOptCfg = $("#rgbchaindev_cfg_textfield").val();
  if (deviceOptCfg.length>0) deviceconfig += ':' + deviceOptCfg;
  closeDialogAndRefreshDevices(apiCall({
    "method":"x-p44-addDevice",
    "dSUID":"none", "x-p44-itemSpec":"vdc:LedChain_Device_Container:1",
    "x": x,
    "dx": dx,
    "y": y,
    "dy": dy,
    "z_order": z_order,
    "uniqueId": uniqueId,
    "deviceConfig": deviceconfig,
    "name" : deviceName
  }));
}



function refresh_dmx_deps()
{
  // current settings
  var dmx_devicetype = $("#dmx_devicetype_select").val();
  // dependent settings
  var show_dimmer = false;
  var show_color = false;
  var show_pos = false;
  switch (dmx_devicetype) {
    case "dimmer" : show_dimmer = true; break;
    case "colordimmer" : show_color = true; break;
    case "movinglight" : show_color = true; show_pos = true; break;
  }
  showIf(show_dimmer, '.dmx_dimmer_group');
  showIf(show_color, '.dmx_color_group');
  showIf(show_pos, '.dmx_position_group');
  setTimeout(function() { $("#dmxCreateDevice").popup("reposition", {positionTo: 'window'}); }, 1 );
}


function createDmxDevice()
{
  // create DMX device
  // - current settings
  var dmx_devicetype = $("#dmx_devicetype_select").val();
  var devicetype = '';
  var deviceconfig = '';
  if (dmx_devicetype=="dimmer") {
    // single channel dimmer, just configure white channel
    devicetype = 'dimmer';
    deviceconfig = 'W=' + $("#dmx_dimmer_channel_textfield").val();
  }
  else if (dmx_devicetype=="colordimmer" || dmx_devicetype=="movinglight") {
    if (
      $("#dmx_red_channel_textfield").val()=='' ||
      $("#dmx_green_channel_textfield").val()=='' ||
      $("#dmx_blue_channel_textfield").val()==''
    ) {
      alert("Please specify DMX channels for R,G,B");
    }
    else {
      // RGB(WA) color
      devicetype = 'color';
      deviceconfig =
        'R=' + $("#dmx_red_channel_textfield").val() + ',' +
        'G=' + $("#dmx_green_channel_textfield").val() + ',' +
        'B=' + $("#dmx_blue_channel_textfield").val();
      if ($("#dmx_white_channel_textfield").val()!='') {
        deviceconfig += ',W=' + $("#dmx_white_channel_textfield").val();
      }
      if ($("#dmx_amber_channel_textfield").val()!='') {
        deviceconfig += ',A=' + $("#dmx_amber_channel_textfield").val();
      }
      if (dmx_devicetype=="movinglight") {
        if ($("#dmx_hpos_channel_textfield").val()!='') {
          deviceconfig += ',H=' + $("#dmx_hpos_channel_textfield").val();
        }
        if ($("#dmx_vpos_channel_textfield").val()!='') {
          deviceconfig += ',V=' + $("#dmx_vpos_channel_textfield").val();
        }
      }
    }
  }
  else {
    // none
  }
  // add extra config
  if ($("#dmx_static_channels_textfield").val()!='') {
    deviceconfig += ',' + $("#dmx_static_channels_textfield").val();
  }
  //alert("devicetype = " + devicetype + "\ndeviceconfig = " + deviceconfig);
  if (devicetype=='' || deviceconfig=='') {
    // need some config
    alert("Please specify required channel numbers");
  }
  else {
    var deviceName = $("#dmxdev_name_textfield").val();
    closeDialogAndRefreshDevices(apiCall({
      "method":"x-p44-addDevice",
      "dSUID":"none", "x-p44-itemSpec":"vdc:OLA_Device_Container:1",
      "deviceType": devicetype,
      "deviceConfig": deviceconfig,
      "name" : deviceName
    }));
  }
}


function createEldatDevice()
{
  // create eldat device
  // - get profile #
  var ety = $("#eldat_type_select").val();
  if (ety<99999) {
    closeDialogAndRefreshDevices(apiCall({
      "method":"x-p44-addProfile",
      "dSUID":"none", "x-p44-itemSpec":"vdc:Eldat_Bus_Container:1",
      "type":ety,
      "address":4294967295 // always automatically assign unused sending channel
    }));
  }
}







function refresh_evaluator_grouping_deps()
{
  var etype = $("#evaluator_devicetype_select").val();
  showIf(etype=="sensor" || etype=="internalsensor", '#evaluator_sensorconfig');
}


function createEvaluatorDevice()
{
  // create evaluator device
  // - get type of evaluator
  var etype = $("#evaluator_devicetype_select").val();
  var ename = $("#evaluator_name_textfield").val();
  if (etype!='none') {
    if (etype == "sensor" || etype == "internalsensor") {
      // sensors must include type and usage
      etype += ':' + $("#evaluator_sensortype_select").val().toString() + ':' + $("#evaluator_sensorusage_select").val().toString();
    }
    apiCall({
      "method":"x-p44-addDevice",
      "dSUID":"none", "x-p44-itemSpec":"vdc:Evaluator_Device_Container:1",
      "evaluatorType":etype,
      "name":ename
    }).done(function(result) {
      closeDialog(function() {
        // get dSUID of new device
        var dSUID = result.dSUID;
        // reset input fields
        $("#evaluator_name_textfield").val('');
        // refresh the list
        refresh_devicelist(dSUID).done(function() {
          // open evaluator editing dialog
          openEvaluatorEditFor(dSUID);
        });
      });
    }).fail(function(domain, code, message) {
      alert("Error: " + message);
    });
  }
}




function refresh_bridgetype_deps()
{
  var shown = $('#bridge_type_select').val().startsWith('scene')
  var on = $('#bridge_onscene_select')
  on.empty()
  var off = $('#bridge_offscene_select')
  off.empty()
  off.append(new Option("- none -", 99999))
  if (shown) {
    sceneslist.forEach(function(o) {
      on.append(new Option(o.name, o.sceneNo));
      off.append(new Option(o.name, o.sceneNo));
    });
    show('#bridge_specificscene_controls')
  }
  else {
    hide('#bridge_specificscene_controls')
  }
}


function createBridgeDevice(activated)
{
  // create bridge device
  // - get bridge type
  var bty = $("#bridge_type_select").val()
  if (bty.startsWith('scene')) {
    bty += ':' + $('#bridge_onscene_select').val() + ':' + $('#bridge_offscene_select').val() + ':' + $('#bridge_resetmode_select').val()
  }
  var bname = $("#bridgedevice_name_textfield").val()
  var group = $("#bridge_initialgroup_select").val()
  if (bty!="0") {
    closeDialogAndRefreshDevices(apiCall({
      "method":"x-p44-addDevice",
      "dSUID":"none", "x-p44-itemSpec":"vdc:Bridge_Device_Container:1",
      "bridgeType":bty,
      "allowBridging": activated,
      "group": group,
      "name":bname
    }))
  }
}




var varDefsHandler = {
  sources: null,
  vardefs: null,

  loadSources: function(forced)
  {
    var self = this;
    if (forced) self.sources = null;
    var dfd = $.Deferred();
    if (self.sources!==null) {
      // already loaded
      dfd.resolve();
    }
    else {
      // query value sources
      apiCall({
        "method":"getProperty",
        "dSUID":"root",
        "query":{
          "x-p44-valueSources":null,
        }
      }).done(function(result) {
        // update value source selector
        self.sources = result['x-p44-valueSources'];
        dfd.resolve();
      }).fail(function(domain, code, message) {
        dfd.reject(domain, code, message);
      });
    }
    return dfd.promise();
  },

  setVarDefs: function(vd)
  {
    this.vardefs = vd;
  },

  varShowHtml: function(vardefs)
  {
    var vhtml = '';
    for (var varname in vardefs) {
      if (vhtml.length>0)
        vhtml += '<br/>';
      var vardef = vardefs[varname];
      vhtml += varname + '(' + escapehtml(vardef.description) + ')';
      if (vardef.age==null)
        vhtml += ' = <span class="evaluationCheckNull">&lt;undefined&gt;</span>';
      else
        vhtml += ' = ' + vardef.value.toString();
    }
    return vhtml;
  },

  renderVarDefs: function(idPrefix)
  {
    var self = this;
    // value definitions
    var bodyId = '#' + idPrefix + 'VarDefsBody';
    var vhtml = '';
    var vd = self.vardefs.split('\n');
    for (var i=0; i<vd.length; ++i) {
      if (vd[i].length>0) {
        var v = vd[i].split(':');
        vhtml +=
          '<tr><td><input autocomplete="off" vsid="' + v[1] + '" value="' + v[0] + '" type="text" data-role="none" /></td>' +
          '<td>' + self.sources[v[1]] + '<a href="#" vsid="' + v[1] + '" + style="float:right" data-role="none">Remove</a></td></tr>';
      }
    }
    // - adder
    vhtml +=
      '<tr><td><label for="' + idPrefix + 'ValueSources">Add input:</label></td>' +
      '<td><select name="' + idPrefix + 'ValueSources" id="' + idPrefix + 'ValueSourcesSelect" data-mini="true" >';
    vhtml += '<option value="-" checked>- select input -</option>';
    // make array and sort it
    var sourcesArray = Object.keys(self.sources).map(function (key) { return { id: key, text: self.sources[key] }; });
    sourcesArray = sourcesArray.sort(function(a,b) { return a.text.localeCompare(b.text); });
    for (var i=0; i<sourcesArray.length; i++) {
      vhtml += '<option value="' + sourcesArray[i].id + '">' + sourcesArray[i].text + '</option>';
    }
    vhtml += '</select></td></tr>';
    $(bodyId).html(vhtml).trigger('create');
    // add handlers
    $(bodyId + ' input').each(function(i,element) {
      $(element).off('change.varDefRename');
      $(element).on('change.varDefRename', function() {
        self.varDefsRenamed(idPrefix);
      });
    });
    $(bodyId + ' a').each(function(i,element) {
      $(element).off('click.varDefRemove');
      $(element).on('click.varDefRemove', function(event) {
        self.removeVarDef(idPrefix, $(event.currentTarget).attr('vsid'));
      });
    });
    $('#' + idPrefix + 'ValueSourcesSelect').off('change.varDefAdd')
    $('#' + idPrefix + 'ValueSourcesSelect').on('change.varDefAdd', function(event) {
      self.addVarDef(idPrefix, event.currentTarget);
    });
  },

  varDefsRenamed: function(idPrefix)
  {
    var self = this;
    // update vardefs
    self.vardefs = '';
    var corrections = false;
    $(".varDefs input").each(function(i, element) {
      var origname = $(element).val().trim();
      var varname = origname.replace(/[^A-Za-z0-9_]/g, "_");
      if (varname!=origname) {
        alert('Invalid sensor name "' + origname + '" changed to "' + varname + '" - sensor names must not contain other characters than A-Z, a-z, 0-9 and _');
        corrections = true;
      }
      if (self.vardefs.length>0) self.vardefs += '\n';
      self.vardefs += varname + ':' + $(element).attr('vsid');
    });
    if (corrections) {
      self.renderVarDefs(idPrefix);
    }
  },

  removeVarDef: function(idPrefix, valueSourceId)
  {
    var vd = this.vardefs.split('\n');
    this.vardefs = '';
    for (var i=0; i<vd.length; ++i) {
      if (!vd[i].includes(valueSourceId)) {
        if (this.vardefs.length>0) this.vardefs += '\n';
        this.vardefs += vd[i];
      }
    }
    this.renderVarDefs(idPrefix);
  },

  addVarDef: function(idPrefix, selector)
  {
    var srcId = $(selector).val();
    if (srcId!='-') {
      var inputIndex = 1;
      while (this.vardefs.includes('Input' + inputIndex.toString())) inputIndex++;
      if (!this.vardefs.includes(srcId)) {
        // not already defined
        if (this.vardefs.includes(':')) this.vardefs = this.vardefs + '\n';
        this.vardefs = this.vardefs + 'Input' + inputIndex.toString() + ':' + srcId;
        this.renderVarDefs(idPrefix);
        $(selector).val('-').selectmenu('refresh'); // reset
      }
    }
  }

} // varDefsHandler


function openEvaluatorEdit(event)
{
  var dSUID = event.currentTarget.getAttribute('dSUID');
  openEvaluatorEditFor(dSUID);
}


function openEvaluatorEditFor(dSUID) {
  varDefsHandler.loadSources(true).done(function() {
    apiCall({
      "method":"getProperty",
      "dSUID":dSUID,
      "query":{
        "deviceIconName":null, "name":null,
        "x-p44-evaluatorType":null,
        "x-p44-varDefs":null,
        "x-p44-onCondition":null,
        "x-p44-offCondition":null,
        "x-p44-minOnTime":null,
        "x-p44-minOffTime":null,
        "x-p44-action":null,
        "x-p44-actionId":null
      }
    }).done(function(evaluator) {
      // determine type
      var isSensor = evaluator['x-p44-evaluatorType']=="sensor" || evaluator['x-p44-evaluatorType']=="internalsensor";
      var hasAction = evaluator['x-p44-evaluatorType']=="internalaction";
      // display dsuid + name
      $('#evaluatorIcon').html('<img src="/icons/icon16/' + evaluator.deviceIconName + '.png" />');
      $('#evaluatorTitle').html(evaluator.name);
      // set current  contents
      // - value defs
      varDefsHandler.setVarDefs(evaluator['x-p44-varDefs']);
      varDefsHandler.renderVarDefs('evaluator');
      $('#evaluatorVarDefsCheck').html('');
      // - conditions / calculation
      if (isSensor) {
        // sensor value calculation
        hide('#evaluatorEdit_boolean');
        show('#evaluatorEdit_sensor');
        $("#evaluatorSensorCalcTextfield").val(evaluator['x-p44-onCondition']);
        $('#evaluatorSensorCalcCheck').html('');
      }
      else {
        // - on/off conditions
        hide('#evaluatorEdit_sensor');
        show('#evaluatorEdit_boolean');
        $("#evaluatorOnConditionTextfield").val(evaluator['x-p44-onCondition']);
        $("#evaluatorOffConditionTextfield").val(evaluator['x-p44-offCondition']);
        $("#evaluatorOnTimeTextfield").val(evaluator['x-p44-minOnTime']);
        $("#evaluatorOffTimeTextfield").val(evaluator['x-p44-minOffTime']);
        $('#evaluatorOnConditionCheck').html('');
        $('#evaluatorOffConditionCheck').html('');
      }
      if (hasAction) {
        show('#evaluatorEdit_action');
        show('#evaluatorEdit_actionHint');
        $("#evaluatorActionTextfield").val(evaluator['x-p44-action']);
        $('#evaluatorActionCheck').html('');
      }
      else {
        hide('#evaluatorEdit_action');
        hide('#evaluatorEdit_actionHint');
        $("#evaluatorActionTextfield").val("");
      }
      activateIDElink('#evaluatorActionScriptEditorLink', evaluator['x-p44-actionId'], function() { return saveEvaluatorData(dSUID, isSensor, true) });
      // set apply handlers
      $("#evaluatorDeviceEditApply").off('click.edit'); // remove previous one
      $("#evaluatorDeviceEditApply").on('click.edit', function () { saveEvaluatorDevice(dSUID, true, isSensor); });
      $("#evaluatorCheckButton").off('click.edit'); // remove previous one
      $("#evaluatorCheckButton").on('click.edit', function() { saveEvaluatorDevice(dSUID, false, isSensor, undefined, event); return false; });
      $("#evaluatorOnActionTestButton").off('click.edit'); // remove previous one
      $("#evaluatorOnActionTestButton").on('click.edit', function() { saveEvaluatorDevice(dSUID, false, isSensor, true, event); return false; });
      $("#evaluatorOffActionTestButton").off('click.edit'); // remove previous one
      $("#evaluatorOffActionTestButton").on('click.edit', function() { saveEvaluatorDevice(dSUID, false, isSensor, false, event); return false; });
      $("#evaluatorActionStopButton").off('click.edit'); // remove previous one
      $("#evaluatorActionStopButton").on('click.edit', function() { stopEvaluatorAction(dSUID, event); return false; });
      // log offset
      $("#evaluatorLogOffset").html(
        '<p><b>Logging:</b>&nbsp;' +
        logOffsetDials(dSUID) +
        '</p>'
      );
      openDialog('#evaluatorDeviceEdit');
    });
  });
}


function saveEvaluatorData(dSUID, isSensor, emptyAsSTX)
{
  var dfd = $.Deferred()
  var src = $("#evaluatorActionTextfield").val()
  if (src.length==0 && emptyAsSTX) src = "\x02";
  if (isSensor) {
    var savequery = {
      "method":"setProperty",
      "dSUID":dSUID,
      "properties":{
        "x-p44-varDefs":varDefsHandler.vardefs,
        "x-p44-onCondition":$("#evaluatorSensorCalcTextfield").val()
      }
    };
  }
  else {
    var savequery = {
      "method":"setProperty",
      "dSUID":dSUID,
      "properties":{
        "x-p44-varDefs":varDefsHandler.vardefs,
        "x-p44-onCondition":$("#evaluatorOnConditionTextfield").val(),
        "x-p44-offCondition":$("#evaluatorOffConditionTextfield").val(),
        "x-p44-minOnTime":$("#evaluatorOnTimeTextfield").val(),
        "x-p44-minOffTime":$("#evaluatorOffTimeTextfield").val(),
        "x-p44-action":src
      }
    };
  }
  apiCall(savequery, 3000).done(function() {
    dfd.resolve()
  })
  return dfd
}


function saveEvaluatorDevice(dSUID, close, isSensor, testState, event)
{
  // save evaluator details
  saveEvaluatorData(dSUID, isSensor).done(function() {
    // check syntax later
    if (close) {
      closeDialog();
    }
    else {
      if (testState!==undefined) {
        // action test
        testEvaluatorAction(dSUID, testState, event);
      }
      else {
        // normal check
        checkEvaluatorDevice(dSUID, isSensor, event);
      }
    }
  });
}


function stopEvaluatorAction(dSUID, event)
{
  var target = getTarget(event)
  apiCall({
    "method":"x-p44-stopEvaluatorAction",
    "dSUID":dSUID
  }).done(function(){
    buttonFeedback(target, 'red')
  });
}


function testEvaluatorAction(dSUID, testState, event)
{
  var target = getTarget(event)
  var actionhtml;
  apiCall({
    "method":"x-p44-testEvaluatorAction",
    "dSUID":dSUID,
    "result":testState
  }).done(function(response) {
    buttonFeedback(target, 'green')
    if (response.error) {
      actionhtml = '<span class="evaluationCheckError">' + escapehtml(response.error) + '</span>';
      if (response.at!==undefined) selectText($('#evaluatorActionTextfield'), response.at, response.at+1);
    }
    else {
      actionhtml = 'Action executed OK';
      if (response.result!=undefined) actionhtml += ' (script return value: <b>' + escapehtml(response.result.toString()) + '</b>)';
    }
  }).fail(function(domain, code, message) {
    actionhtml = '<span class="evaluationCheckError">' + escapehtml(message) + '</span>';
  }).always(function() {
    $('#evaluatorActionCheck').html(actionhtml).trigger('create');
  });
}


function checkEvaluatorDevice(dSUID, isSensor, event)
{
  var target = getTarget(event)
  // check evaluator syntax and results
  apiCall({
    "method":"x-p44-checkEvaluator",
    "dSUID":dSUID
  }).done(function(result) {
    buttonFeedback(target, 'green');
    // variable definitions and current values
    $('#evaluatorVarDefsCheck').html(varDefsHandler.varShowHtml(result.varDefs)).trigger('create');
    // on condition
    var condhtml;
    var cond = result.onCondition;
    if (isSensor) {
      // on condition is value calculation
      if (cond.error) {
        condhtml = '<span class="evaluationCheckError">' + escapehtml(cond.error) + '</span>';
        if (cond.at!==undefined) selectText($('#evaluatorSensorCalcTextfield'), cond.at, cond.at+1);
      }
      else if (cond.result==null) {
        condhtml = '<span class="evaluationCheckNull">' + escapehtml(cond.text) + '</span>';
      }
      else {
        condhtml = 'Expression evaluates to <b>' + escapehtml(cond.text) + '</b>';
      }
      $('#evaluatorSensorCalcCheck').html(condhtml).trigger('create');
    }
    else {
      // on and off conditions
      if (cond.error) {
        condhtml = '<span class="evaluationCheckError">' + escapehtml(cond.error) + '</span>';
        if (cond.at!==undefined) selectText($('#evaluatorOnConditionTextfield'), cond.at, cond.at+1);
      }
      else if (cond.result==null) {
        condhtml = '<span class="evaluationCheckNull">' + escapehtml(cond.text) + '</span>';
      }
      else {
        condhtml = 'Expression evaluates to <b>' + escapehtml(cond.text) + '</b> (' + (cond.result>0 ? 'true' : 'false') + ')';
      }
      $('#evaluatorOnConditionCheck').html(condhtml).trigger('create');
      // - also display off condition
      var cond2 = result.offCondition;
      if (cond2.error) {
        condhtml = '<span class="evaluationCheckError">' + escapehtml(cond2.error) + '</span>';
        if (!cond.error && cond2.at!=undefined) selectText($('#evaluatorOffConditionTextfield'), cond2.at, cond2.at+1); // only if no error in first expression
      }
      else if (cond2.result==null) {
        condhtml = '<span class="evaluationCheckNull">' + escapehtml(cond2.text) + '</span>';
      }
      else {
        condhtml = 'Expression evaluates to <b>' + escapehtml(cond2.text) + '</b> (' + (cond2.result>0 ? 'true' : 'false') + ')';
      }
      $('#evaluatorOffConditionCheck').html(condhtml).trigger('create');
    }
  });
}



function createScriptedDevice()
{
  // create scripted custom device
  var init = $("#scripteddevInitmsgTextfield").val();
  apiCall({
    "method":"x-p44-addDevice",
    "dSUID":"none", "x-p44-itemSpec":"vdc:Scripted_Device_Container:1",
    "init":init.toString()
  }).done(function(result) {
    closeDialog(function() {
      // get dSUID of new device
      var dSUID = result.dSUID;
      // reset input fields
      $("#scripteddevInitmsgTextfield").val('');
      // refresh the list
      refresh_devicelist(dSUID).done(function() {
        // open evaluator editing dialog
        openScriptedDeviceEditFor(dSUID);
      });
    });
  }).fail(function(domain, code, message) {
    alert("Error: " + message);
  });
}


function openScriptedDeviceEdit(event)
{
  var dSUID = event.currentTarget.getAttribute('dSUID');
  openScriptedDeviceEditFor(dSUID);
}


function openScriptedDeviceEditFor(dSUID) {
  apiCall({
    "method":"getProperty",
    "dSUID":dSUID,
    "query":{
      "deviceIconName":null, "name":null,
      "x-p44-implementation":null,
      "x-p44-implementationId":null,
      "x-p44-initmessage":null
    }
  }).done(function(scripted) {
    // display dsuid + name
    $('#scriptedDeviceIcon').html('<img src="/icons/icon16/' + scripted.deviceIconName + '.png" />');
    $('#scriptedDeviceTitle').html(scripted.name);
    // display init message for reference
    $("#scriptedDeviceInitMessage").html(escapehtml(scripted['x-p44-initmessage']));
    // set current  contents
    $("#scriptedDeviceImplementationTextfield").val(scripted['x-p44-implementation']);
    $('#scriptedDeviceCheckResult').html('');
    activateIDElink('#scriptedDeviceScriptEditorLink', scripted['x-p44-implementationId'], function() { return saveScriptedDeviceData(dSUID, true) });
    // set apply handlers
    $("#scriptedDeviceEditApply").off('click.edit'); // remove previous one
    $("#scriptedDeviceEditApply").on('click.edit', function () { saveAndRestartScriptedDevice(dSUID, true, true); });
    $("#scriptedDeviceRestartButton").off('click.edit'); // remove previous one
    $("#scriptedDeviceRestartButton").on('click.edit', function() { saveAndRestartScriptedDevice(dSUID, false, true); return false; });
    $("#scriptedDeviceStopButton").off('click.edit'); // remove previous one
    $("#scriptedDeviceStopButton").on('click.edit', function() { stopImplementation(dSUID); return false; });
    // log offset
    $("#scriptedDeviceLogOffset").html(
      '<p><b>Logging:</b>&nbsp;' +
      logOffsetDials(dSUID) +
      '</p>'
    );
    openDialog('#scriptedDeviceEdit');
  });
}


function saveScriptedDeviceData(dSUID, emptyAsSTX)
{
  var dfd = $.Deferred()
  var src = $("#scriptedDeviceImplementationTextfield").val()
  if (src.length==0 && emptyAsSTX) src = "\x02";
  var savequery = {
    "method":"setProperty",
    "dSUID":dSUID,
    "properties":{
      "x-p44-implementation":src
    }
  };
  apiCall(savequery, 3000).done(function() {
    dfd.resolve()
  })
  return dfd
}


function saveAndRestartScriptedDevice(dSUID, close, restart)
{
  // save scripted device details
  saveScriptedDeviceData(dSUID).done(function() {
    // check syntax
    apiCall({
      "method":"x-p44-checkImpl",
      "dSUID":dSUID
    }).done(function(response) {
      var msghtml = '';
      if (response.error) {
        msghtml = 'Error: <span class="error">' + escapehtml(response.error) + '</span>';
        if (response.at!==undefined) {
          selectText($('#scriptedDeviceImplementationTextfield'), response.at, response.at+1);
        }
        if (close) {
          alert("Error: " + response.error);
          closeDialog();
        }
      }
      else {
        // Script is ok
        msghtml = "Syntax OK";
        if (restart) {
          msghtml += " - Restarted";
          apiCall({
            "method":"x-p44-restartImpl",
            "dSUID":dSUID
          });
        }
        if (close) {
          closeDialog();
        }
      }
      $("#scriptedDeviceCheckResult").html(msghtml);
    }).fail(function(domain, code, message) {
      $("#scriptedDeviceCheckResult").html('<span class="error">API error: [' + escapehtml(domain + '] Error ' + code.toString() + ': ' + message) + '</span>');
      if (close) {
        alert("Error: " + message);
        closeDialog();
      }
    });
  });
}



function stopImplementation(dSUID)
{
  apiCall({
    "method":"x-p44-stopImpl",
    "dSUID":dSUID
  }).always(function() {
    $("#scriptedDeviceCheckResult").html('<span class="error">Stopped</span>');
  })
}




function removeDevice(dSUID)
{
  openDialog("#removeDeviceConfirm", function() {
    $("#removeDeviceNowButton").off('click.delete');
    $("#removeDeviceNowButton").on('click.delete', function() {
      removeDeviceNow(dSUID);
    });
  });
}


function removeDeviceNow(dSUID)
{
  // remove now
  apiCall({
    "method":"x-p44-removeDevice",
    "dSUID":dSUID
  }).always(function() {
    // reload modified device list
    closeDialog(function () {
      refresh_devicelist();
    });
  });
}


function getAttention(dSUID)
{
  // blink
  apiCall({
    "notification":"identify",
    "dSUID":dSUID
  }, 1000);
}


function openProperties(dSUID)
{
  // show device properties in separate page
  window.open("/props.html#" + addSession(dSUID), '_blank');
}


function openTweak()
{
  window.open("/tweak.html#" + addSession(''), '_blank');
}


function openMainScript()
{
  openScriptInIDE('mainscript');
}


function openREPL()
{
  window.open("/repl.html#" + addSession(''), '_blank');
}


var scriptRefWindow = undefined;

function openScriptRef()
{
  if (!scriptRefWindow || scriptRefWindow.closed) {
    scriptRefWindow = window.open('/script_ref.html', 'p44script-reference');
  }
  else {
    scriptRefWindow.focus();
  }
}


var ledSimWindow = undefined;

function openLEDSim()
{
  if (!ledSimWindow || ledSimWindow.closed) {
    ledSimWindow = window.open('/ledsim.html', 'p44lrg-debugger');
  }
  else {
    ledSimWindow.focus();
  }
}


var ideWindow = undefined;

function openScriptInIDE(scriptid)
{
  if (scriptid) {
    document.cookie = "scriptToOpen=" + scriptid
  }
  if (!ideWindow || ideWindow.closed) {
    ideWindow = window.open('/ide.html', 'p44script-IDE');
  }
  else {
    ideWindow.focus();
  }
}


function activateIDElink(selector, scriptidOrCB, saveCB)
{
  $(selector).off()
  $(selector).on('click', function(event) {
    // save what we have in the current dialog
    if (saveCB) saveCB().done(function() {
      // need the id to open in the IDE
      if (typeof scriptidOrCB == "function") {
        // this is a getter for the scriptID, call it
        scriptidOrCB().done(function(scriptid) {
          console.log(`activateIDElink: id from getter = ${scriptid}`)
          openScriptInIDE(scriptid)
          closeDialog()
        })
      }
      else {
        // id already present, just use it
        console.log(`activateIDElink: direct id = ${scriptidOrCB}`)
        openScriptInIDE(scriptidOrCB)
        closeDialog()
      }
    })
  })
}


function openDevTabs()
{
  // IDE with mainscript open is a good starting point
  openMainScript();
}


function openVdcInfo(event, hiddenFeatures)
{
  var dSUID = event.currentTarget.getAttribute('dSUID');
  if (hiddenFeatures==2) {
    if (event.metaKey || event.ctrlKey) {
      // show vdc properties in separate page
      openProperties(dSUID)
      return;
    }
  }
  var alternativeDialog = (hiddenFeatures==1) || (event.shiftKey && !event.altKey);
  hiddenFeatures ||= event.shiftKey;
  // vdc properties
  $("#vdcOpenPropsLink").off();
  $("#vdcOpenPropsLink").on('click', function() {
    openProperties(dSUID);
  });
  // query vdc details
  apiCall({
    "method":"getProperty",
    "dSUID":dSUID,
    "query":{
      "deviceIconName":null, "dSUID":null, "name":null, "model":null, "modelVersion":null,
      "displayId":null, "vendorName":null, "modelUID":null,
      "x-p44-rescanModes":null, "x-p44-optimizerMode":null, "x-p44-extraInfo":null, "x-p44-description":null, "x-p44-hideWhenEmpty":null,
      "implementationId":null,
      "capabilities":{ "identification": null }
    }
  }).done(function(vdc) {
    // check for hidden alternative dialogs for creating devices
    if (alternativeDialog) {
      if (vdc['implementationId'] == 'EnOcean_Bus_Container') {
        // show hidden enocean profile creation dialog
        openDialog("#enoceanCreateDevice");
        return; // done
      }
      if (vdc['implementationId'] == 'Eldat_Bus_Container') {
        // show hidden eldat profile creation dialog
        openDialog("#eldatCreateDevice");
        return; // done
      }
    }
    // normal info box display
    $('#vdcInfoIcon').html('<img src="/icons/icon16/' + vdc.deviceIconName + '.png" />');
    $('#vdcInfoTitle').html(vdc.name);
    hide('#vdcTechDetails');
    $('#vdcP44Description').html(escapehtml(vdc['x-p44-description']));
    var infoHTML =
      '<table id="vdcInfoTable">' +
      '<tr><td width="35%">vdc dSUID</td><td class="infovalue">' + vdc.dSUID + '</td></tr>' +
      '<tr><td>dS modelUID</td><td class="infovalue">' + vdc.modelUID + '</td></tr>' +
      '<tr><td>Model</td><td class="infovalue">' + dispStr(vdc.model) + '</td></tr>' +
      '<tr><td>Version</td><td class="infovalue">' + dispStr(vdc.modelVersion) + '</td></tr>' +
      '<tr><td>Hardware ID</td><td class="infovalue">' + dispStr(vdc.displayId) + '</td></tr>' +
      '<tr><td>Vendor</td><td class="infovalue">' + dispStr(vdc.vendorName) + '</td></tr>';
    if (vdc['x-p44-extraInfo']) {
      infoHTML += '<tr><td>vdc specific</td><td class="infovalue">' + vdc['x-p44-extraInfo'] + '</td></tr>';
    }
    if (hiddenFeatures) {
      infoHTML +=
        '<tr><td>Logging</td><td class="infovalue">' +
        logOffsetDials(vdc.dSUID) +
        '</td></tr>';
    }
    infoHTML += '</table>';
    $('#vdcInfoShow').html(infoHTML);
    // probably add some extra vdc-specific controls
    var extracontrols='';
    if (vdc.capabilities.identification) {
      // identification available
      extracontrols +=
        '<button onclick="getAttention(\'' + vdc.dSUID + '\');" type="button" id="deviceInfoBlink" data-icon="check">Identify (e.g. blink)</button>';
    }
    if (vdc['x-p44-rescanModes'] != 0) {
      // rescan available
      extracontrols +=
        '<button onclick="askRescanVdc(\'' + vdc.dSUID + '\',' + vdc['x-p44-rescanModes'] + ', \'' + vdc['implementationId'] + '\');" type="button" id="vdcRescan" >Scan for devices...</button>';
    }
    if (vdc['implementationId'] == 'DALI_Bus_Container') {
      // DALI specific
      extracontrols +=
        '<button onclick="daliDiagnostics(\'' + vdc.dSUID + '\');" type="button" id="daliMonitor" >DALI bus diagnostics...</button>';
      extracontrols +=
        '<a data-role="button" href="/daliplan.html#' + addSession('') + '" target="_blank" id="daliPlan" >DALI hardware summary...</a>';
    }
    if (vdc['implementationId'] == 'hue_Lights_Container') {
      // hue specific
      extracontrols +=
        '<button onclick="hueBridgeOptions(\'' + vdc.dSUID + '\');" type="button" id="hueAPIEnter" >Connect hue bridge via IP...</button>';
    }
    if (typeof(vdc['x-p44-optimizerMode'])!="undefined") {
      var m = vdc['x-p44-optimizerMode'];
      extracontrols +=
        '<p>&nbsp;</p>' +
        '<label for="optimizerMode">Optimize scene calls and/or dimming:</label>' +
        '<select name="optimizerMode" data-mini="true" id="optimizerMode_select" onchange="optimizerChanged(\''  + vdc.dSUID + '\')">' +
          '<option ' + optValSel(1,m) + '>Optimization disabled</option>' +
          '<option ' + optValSel(2,m) + '>Use existing optimizations, no changes</option>' +
          '<option ' + optValSel(3,m) + '>Use existing optimizations, update when scenes are changed/saved</option>' +
          '<option ' + optValSel(4,m) + '>Automatically create optimizations based on usage statistics</option>' +
          '<option ' + optValSel(5,m) + '>Clear optimizer cache (but no mode change) - use with care!</option>' +
        '</select>';
    }
    $('#vdcInfoExtraControls').html(extracontrols).trigger('create');
    openDialog("#vdcInfo");
  });
}


function optimizerChanged(dSUID)
{
  var mode = $('#optimizerMode_select').val();
  setOptimizerMode(dSUID,mode);
}

function setOptimizerMode(dSUID,mode)
{
  apiCall({
    "method":"setProperty",
    "dSUID":dSUID,
    "properties":{
      "x-p44-optimizerMode":mode
    }
  });
}




function askRescanVdc(dSUID, modes, implementationId)
{
  // close the vdc info
  closeDialog(function() {
    // show rescan choices
    var scanButtons='';
    if (modes & 0x01) {
      // has incremental rescan
      scanButtons +=
        '<button onclick="rescanVdc(\'' + dSUID + '\', true, false, false, false);" type="button" id="vdcRescan_i" >Look for new devices only</button>';
    }
    if (modes & 0x02) {
      // has normal rescan
      scanButtons +=
        '<button onclick="rescanVdc(\'' + dSUID + '\', false, false, false, false);" type="button" id="vdcRescan_n" >Normally re-scan and re-register all devices</button>';
    }
    if (modes & 0x04) {
      // has more dangerous modes as well, hide them to begin with
      scanButtons +=
        '<button onclick="showDangerousRescans();" type="button" id="vdcRescan_d" >Service-only scans...</button>' +
        '<div id="vdcDangerousScanButtons" style="display:none">';
      // has exhaustive rescan (show clearconfig and enumerate only together with exhaustive scan)
      if (implementationId == 'DALI_Bus_Container') {
        scanButtons +=
          '<p><span style="color:red">Warning: These service scans can cause DALI short-address reassignments!</span> Use with care when you have DALI devices that are not fully compatible (no unique serial number) - dSUIDs might change for those. Generally, use only when you have DALI address conflicts!</p>';
      }
      else {
        scanButtons +=
          '<p><span style="color:red">Use with caution!</span> Do not use these extended rescans without a specific reason!</p>';
      }
      scanButtons +=
        '<button onclick="rescanVdc(\'' + dSUID + '\', false, true, false, false);" type="button" id="vdcRescan_f" >Force full re-scan of all devices</button>';
      if (modes & 0x10) {
        // has re-enumeration
        scanButtons +=
          '<p><span style="color:red">Warning: Re-enumerating might change/swap dSUIDs for devices without a stable electronic serial number!</span> Only use when you really want to completely restart bus addressing!</p>' +
          '<button onclick="rescanVdc(\'' + dSUID + '\', false, true, false, true);" type="button" id="vdcRescan_fe" >Re-enumerate devices and fully re-scan all devices</button>';
      }
      // clear config is always available as an option
      scanButtons +=
        '<p><span style="color:red">Warning: Use clearing settings with caution!</span>  All configuration for the current devices will be lost; devices will re-appear in default state and without name after scan.</span></p>' +
        '<button onclick="rescanVdc(\'' + dSUID + '\', false, true, true, false);" type="button" id="vdcRescan_fc" >Clear settings and fully re-scan all devices</button>';
      // end of dangerous options
      scanButtons += '</div>';
    }
    $('#vdcScanButtons').html(scanButtons).trigger('create');
    openDialog('#vdcScanChoices');
  });
}


function showDangerousRescans()
{
  show('#vdcDangerousScanButtons');
  $('#vdcScanChoices').popup("reposition", {positionTo: 'window'});
}


function rescanVdc(dSUID, incremental, exhaustive, clear, reenumerate)
{
  // rescan
  $.mobile.loading( "show", {
    text: "Scanning for devices...",
    textVisible: true,
    theme: "b"
  });
  // long timeout: 5min!
  closeDialogAndRefreshDevices(apiCall({
    "method":"scanDevices",
    "dSUID":dSUID,
    "incremental":incremental,
    "exhaustive":exhaustive,
    "reenumerate":reenumerate,
    "clearconfig":clear
  }, 1200000));
}



function logOffsetDials(dSUID)
{
  return '<a title="min logging" href="javascript:set_logoffs(\'' + dSUID + '\',-2);">-2</a>&nbsp;' +
    '<a title="less logging" href="javascript:set_logoffs(\'' + dSUID + '\',-1);">-1</a>&nbsp;' +
    '<a title="default logging" href="javascript:set_logoffs(\'' + dSUID + '\',0);">normal</a>&nbsp;' +
    '<a title="more logging" href="javascript:set_logoffs(\'' + dSUID + '\',1);">+1</a>&nbsp;' +
    '<a title="max logging" href="javascript:set_logoffs(\'' + dSUID + '\',2);">+2</a>';
}


function set_logoffs(dSUID, offset)
{
  apiCall({
    "method":"setProperty",
    "dSUID":dSUID,
    "properties":{ "x-p44-logLevelOffset":offset }
  });
}


var deviceInfoUpdaterTimeout = 0;
var deviceInfoIsOpen = false;



function enableForBridging(event, enable)
{
  var dSUID = event.currentTarget.getAttribute('dSUID');
  apiCall({
    "method":"setProperty",
    "dSUID":dSUID,
    "properties":{ "x-p44-allowBridging":enable }
  }).done(function() {
    // update bridging info
    apiCall({
      "method":"getProperty",
      "dSUID":dSUID,
      "query":{ "x-p44-allowBridging":null, "x-p44-bridged":null }
    }).done(function(device) {
      $('#deviceBridgingControls').html(
        bridgecontrols(dSUID, device['x-p44-allowBridging'], device['x-p44-bridged'])
      ).trigger('create')
    })
  })
}


function bridgecontrols(dSUID, allow, bridged)
{
  var html = '';
  if (bridgeinfo && bridgeinfo['connected']) {
    if (bridgeinfo.bridgetype=='proxy') {
      html +=
        '<p style="padding-top:12px;"></p>' +
        (allow
        ? '<button dSUID="' + dSUID + '" onclick="enableForBridging(event, false);" type="button" id="deviceInfoEnableBridging" data-icon="home">Disable proxy for this device </button>'
        : '<button dSUID="' + dSUID + '" onclick="enableForBridging(event, true);" type="button" id="deviceInfoEnableBridging" data-icon="home">Enable proxy for this device </button>') +
        '<div id="bridgedDeviceInfo" style="display:' + (allow ? "block": "none") + '" data-mini="true">' +
        'Proxy status: <b>' + (bridged ? '<span class="bridgedIndicator">connected to proxy</span>' : 'ready for proxy to connect') + '</b></div>';
    }
    else {
      // assume matter, older p44mbrd do not set bridgetype
      html +=
        '<p style="padding-top:12px;"></p>' +
        (allow
        ? '<button dSUID="' + dSUID + '" onclick="enableForBridging(event, false);" type="button" id="deviceInfoEnableBridging" data-icon="home">Stop bridging to matter </button>'
        : '<button dSUID="' + dSUID + '" onclick="enableForBridging(event, true);" type="button" id="deviceInfoEnableBridging" data-icon="home">Enable for bridging to matter </button>') +
        '<div id="bridgedDeviceInfo" style="display:' + (allow ? "block": "none") + '" data-mini="true">' +
        'Bridged device status: <b>' + (bridged ? '<span class="bridgedIndicator">connected to matter bridge</span>' : 'ready for bridge to connect') + '</b></div>';
    }
  }
  return html;
}


function restartMatterBridge(exitcode)
{
  apiCall({
    "method":"x-p44-notifyBridge",
    "dSUID":"root",
    "bridgenotification":"terminate",
    "exitcode": exitcode
  }).done(function(device) {
    closeDialog()
  })
}



function setBridgeLoglevel(app,chip)
{
  req = {
    "method":"x-p44-notifyBridge",
    "dSUID":"root",
    "bridgenotification":"loglevel",
  }
  if (app!==undefined) req.app = app;
  if (chip!==undefined) req.chip = chip;
  apiCall(req).done(function(device) {
    $('#deviceBridgingControls').html(
      bridgecontrols(dSUID, device['x-p44-allowBridging'], device['x-p44-bridged'])
    ).trigger('create')
  })
}


function requestCommissioning(enable)
{
  req = {
    "method":"x-p44-notifyBridge",
    "dSUID":"root",
    "bridgenotification":"commissioning",
  }
  req.enable = enable;
  apiCall(req).done(function(device) {
    closeDialog(function() {
      setTimeout(openMatterBridgeConfig, 2000)
    })
  })
}


function enableMatterBridge(enable)
{
  p44mCall({
    "cmd":"property",
    "key":"p44mbrd",
    "value":enable ? 1 : 0
  }).always(function() {
    system_restart()
  })
}


function refresh_bridgeinfo()
{
  var dfd = $.Deferred();
  // update global bridgeinfo
  apiCall({
    "method":"getProperty",
    "dSUID":"root",
    "query":{ "x-p44-bridge":null }
  }).done(function(result) {
    bridgeinfo = result['x-p44-bridge'];
  }).fail(function() {
    bridgeinfo = {};
  }).always(function() {
    dfd.resolve();
  });
  return dfd.promise();
}


function openMatterBridgeConfig()
{
  refresh_bridgeinfo().done(function() {
    if (bridgeinfo) {
      if (!bridgeinfo.connected) {
        // matter daemon not yet running (and no proxy connected)
        // - the only regular cause for this is not having the enable flag set, so show first-time enable dialog
        openDialog('#matterBridgeEnable')
        return;
      }
      openDialog('#matterBridgeConfig', function() {
        if (bridgeinfo.bridgetype=='proxy') {
          hide('#matterBridgeButtons');
          $('#matterBridgeInfo').html(
            '<p><span class="bridgedIndicator">Another P44-xx is connected as a proxy to supervise some of the local devices</span></p>' +
            '<abbr title="Open WebUI of supervising P44 device which hosts proxies of some of the local devices"><a target="_blank" href="' + bridgeinfo.configURL + '" data-role="button" data-icon="arrow-r" data-iconpos="left">Open Proxy WebUI</a></abbr>'
          ).trigger("create")
        }
        else {
          // assume matter
          show('#matterBridgeButtons');
          if (bridgeinfo.connected) {
            if (!bridgeinfo.started) {
              $('#matterBridgeInfo').html(
                '<p><span class="bridgedIndicator">Matter briding/commissioning can start only with devices enabled for briding. ' +
                'Please enable at least one device for bridging, or create a room state bridging device. Then return here to see ' +
                'the onboarding QR code.</span></p>'
              ).trigger("create")
            }
            else if (bridgeinfo.commissionable) {
              $('#matterBridgeInfo').html(
                '<p>Scan the the QR code with your Matter commissioning app (e.g. Apple Home.app)</p>' +
                '<div id="onboarding_qrcode"></div>' +
                '<div id="manualpairingcode">Manual pairing code: <b>' + bridgeinfo.manualpairingcode + '</b></div>' +
                '<button type="button" onclick="requestCommissioning(false);" data-icon="minus">End Pairing Mode</button>'
              ).trigger("create")
              $('#onboarding_qrcode').html('')
              var qrcode = new QRCode(document.getElementById("onboarding_qrcode"), {
                width : 200,
                height : 200
              });
              qrcode.makeCode(bridgeinfo.qrcodedata);
            }
            else {
              $('#matterBridgeInfo').html(
                '<p><span class="bridgedIndicator">Matter Bridge is not in commissioning mode (and already commissioned into at least one fabric)</span></p>' +
                '<button type="button" onclick="requestCommissioning(true);" data-icon="plus">Turn On Pairing Mode</button>'
              ).trigger("create")
            }
          }
          else {
            $('#matterBridgeInfo').html('<p>bridge not running</p>').trigger("create")
          }
        }
        setTimeout(function() { $("#matterBridgeConfig").popup("reposition", {positionTo: 'window'}); }, 1 );
      })
    }
  })
}



function openDeviceInfo(event, hiddenFeatures)
{
  var dSUID = event.currentTarget.getAttribute('dSUID');
  if (hiddenFeatures==2) {
    openProperties(dSUID);
    return;
  }
  hiddenFeatures ||= event.shiftKey;
  deviceInfoIsOpen = true;
  // stop updating inputs when popup closes!
  $("#deviceInfo").off('popupafterclose.devinfo');
  $("#deviceInfo").on('popupafterclose.devinfo', function() {
    clearTimeout(deviceInfoUpdaterTimeout);
    deviceInfoIsOpen = false;
  });
  // device properties
  $("#deviceOpenPropsLink").off();
  $("#deviceOpenPropsLink").on('click', function() {
    openProperties(dSUID);
  });
  // query device details
  alertError(apiCall({
    "method":"getProperty",
    "dSUID":dSUID,
    "query":{
      "deviceIconName":null, "dSUID":null, "name":null, "model":null, "modelVersion":null,
      "displayId":null, "vendorName":null, "modelUID":null, "hardwareModelGuid":null,
      "implementationId":null,
      "x-p44-softwareRemovable":null, "x-p44-extraInfo":null, "configurationDescriptions":null, "configurationId":null, "x-p44-teachInSignals":null,
      "x-p44-opStateLevel":null, "x-p44-opStateText":null, "x-p44-description":null,
      "x-p44-bridgeable":null, "x-p44-bridged":null, "x-p44-allowBridging":null,
      "buttonInputDescriptions":{"#":null}, "buttonInputSettings":{ "#0":null }, "binaryInputDescriptions":{"#":null}, "channelDescriptions":{"#":null}, "sensorDescriptions":{"#":null},
      "modelFeatures":{ "identification":null },
      "zoneID":null
    }
  }, 3000)).done(function(device) {
    $('#deviceInfoIcon').html('<img src="/icons/icon16/' + device.deviceIconName + '.png" />');
    $('#deviceInfoTitle').html(device.name);
    hide('#deviceTechDetails');
    $('#deviceP44Description').html(escapehtml(device['x-p44-description']));
    var infoHTML =
      '<table id="deviceInfoTable">' +
      '<tr><td width="35%">dSUID</td><td class="infovalue">' + device.dSUID + '</td></tr>' +
      '<tr><td>dS modelUID</td><td class="infovalue">' + device.modelUID + '</td></tr>' +
      '<tr><td>Model</td><td class="infovalue">' + device.model + '</td></tr>' +
      '<tr><td>Version</td><td class="infovalue">' + dispStr(device.modelVersion) + '</td></tr>' +
      '<tr><td>Hardware ID</td><td class="infovalue">' + dispStr(device.displayId) + '</td></tr>';
    if (device['implementationId'].substr(0,5)=='dali_' && device.hardwareModelGuid && device.hardwareModelGuid.substr(0,8)=="gs1:(01)") {
      // DALI GTIN lookup
      var gtinlookupurl = 'https://www.dali-alliance.org/products?gtin=' + device.hardwareModelGuid.substr(8) + '&registered%5B%5D=';
      infoHTML += '<tr><td>&nbsp;</td><td><a target="_blank" href="' + gtinlookupurl + '">Look up in DALI device database</a></td></tr>';
    }
    infoHTML +=
      '<tr><td>Vendor</td><td class="infovalue">' + dispStr(device.vendorName) + '</td></tr>';
    if (typeof(device['x-p44-opStateText'])!="undefined") {
      infoHTML += '<tr><td>Status</td><td id="deviceStatusInfo" class="infovalue"></td></tr>';
      // might be volatile, schedule first update - will be repeated periodically as long as dialog is open
      deviceInfoUpdaterTimeout = setTimeout(function() { updateDeviceInfo(dSUID); }, 100);
    }
    if (device['x-p44-extraInfo']) {
      infoHTML += '<tr><td>Device specific</td><td id="deviceInfoExtra" class="infovalue">' + device['x-p44-extraInfo'] + '</td></tr>';
    }
    if (hiddenFeatures) {
      infoHTML +=
        '<tr><td>Logging</td><td class="infovalue">' +
        logOffsetDials(device.dSUID) +
        '</td></tr>';
    }
    infoHTML += '</table>';
    $('#deviceInfoShow').html(infoHTML);
    // probably add some extra device-specific controls
    var extracontrols='';
    if (device.modelFeatures.identification) {
      // identification available
      extracontrols +=
        '<button onclick="getAttention(\'' + device.dSUID + '\');" type="button" id="deviceInfoBlink" data-icon="check">Identify (e.g. blink)</button>';
    }
    if (device['implementationId']=='dali_rgbw') {
      // extra ungroup button for grouped composite device
      extracontrols +=
        '<button onclick="ungroupDaliDevices(\'' + device.dSUID + '\');" type="button" id="deviceInfoUngroup" data-icon="delete">Ungroup color composite device</button>';
    }
    else if (device['implementationId']=='dali_group') {
      // extra ungroup button for grouped DALI dimmers
      extracontrols +=
        '<button onclick="ungroupDaliDevices(\'' + device.dSUID + '\');" type="button" id="deviceInfoUngroup" data-icon="delete">Ungroup dimmers</button>';
    }
    if (device['implementationId'].substr(0,5)=='dali_' && device['implementationId']!='dali_input') {
      // extra button for setting default brightness
      extracontrols +=
        '<button onclick="setDALIDefaultBrightness(\'' + device.dSUID + '\');" type="button" id="deviceInfoUngroup" data-icon="arrow-d">Set power-on/default brightness...</button>';
    }
    if (device['implementationId']=='evaluator') {
      // evaluator edit button
      extracontrols +=
        '<button dSUID="' + device.dSUID + '" onclick="openEvaluatorEdit(event);" type="button" id="deviceInfoEditEvaluator" data-icon="edit">Edit calculations...</button>';
    }
    if (device['implementationId']=='scripted') {
      // script implementatuon edit button
      extracontrols +=
        '<button dSUID="' + device.dSUID + '" onclick="openScriptedDeviceEdit(event);" type="button" id="deviceInfoEditScripted" data-icon="edit">Edit implementation script...</button>';
    }
    // rename device
    extracontrols +=
      '<button onclick="openRenameAddressable(\'' + device.dSUID + '\');" type="button" id="deviceInfoRenameDevice" data-icon="edit">Rename...</button>';
    // button settings
    if (device.buttonInputDescriptions['#']>0) {
      extracontrols +=
        '<button dSUID="' + device.dSUID + '" onclick="openButtonSettings(event);" type="button" id="deviceInfoButtonSettings" data-icon="grid">Button Settings...</button>';
    }
    // change zone
    extracontrols +=
      '<p>Zone:</p>' +
      zoneselector('deviceZoneSelector', device.zoneID, true);
    if (device['x-p44-softwareRemovable']==true) {
      // removable static device, add extra remove button
      extracontrols +=
        '<p>&nbsp;</p>' +
        '<button onclick="removeDevice(\'' + device.dSUID + '\');" type="button" id="deviceInfoRemove" data-icon="delete">Remove device...</button>';
    }
    if (device['x-p44-teachInSignals']>0) {
      // device has teach-in signals
      for(var k = 0; k<device['x-p44-teachInSignals']; k++) {
        extracontrols +=
          '<button onclick="event.stopPropagation(); sendTeachInSignal(\'' + device.dSUID + '\', ' + k.toString() + ');" type="button" data-mini="true" data-inline="true" data-icon="gear">Teach-in #' + k.toString() + '</button>';
      }
    }
    var configurationVariants = device['configurationDescriptions'];
    if (configurationVariants!=undefined) {
      var numconfigs = 0;
      var configs = '<p style="padding-top:20px;"></p>';
      configs += '<label for="configurationVariants">Device has multiple configurations:</label><select name="configurationVariants"  data-mini="true" id="configurationVariants_select" onchange="show(\'#deviceInfoConfigurationApply\')">';
      for (var configurationId in configurationVariants) {
        configs += '<option ' + optValSel(configurationId.toString(), device['configurationId']) + '>' + configurationVariants[configurationId].description + '</option>';
        numconfigs++;
      }
      configs += '</select>';
      // - apply button (hidden until selector changed)
      configs += '<div id="deviceInfoConfigurationApply" style="display:none">';
      configs += '<p><span style="color:red">Please make sure the selected configuration is suitable for the device you have connected before applying!</span></p>';
      configs += '<button onclick="event.stopPropagation(); setDeviceConfiguration(\'' + device.dSUID + '\');" type="button" id="deviceInfoSetConfiguration" data-theme="d" data-mini="true" data-icon="check">Apply new configuration</button>';
      configs += '</div>';
      if (numconfigs>1) {
        extracontrols += configs;
      }
    }
    $('#deviceInfoExtraControls').html(extracontrols).trigger('create');
    // update bridging info
    $('#deviceBridgingControls').html(bridgecontrols(dSUID, device['x-p44-allowBridging'], device['x-p44-bridged'])).trigger('create');
    // bind handlers
    $('#deviceZoneSelector').on('change.devicezone', function(event) {
      // zone selected
      var zoneid = $('#' + event.currentTarget.id).val();
      if (zoneid==-1) {
        createAndAssignNewZone(device.dSUID);
      }
      else if (zoneid==-2) {
        editZones();
      }
      else {
        setDeviceZone(device.dSUID, zoneid);
      }
    });
    openDialog("#deviceInfo");
  });
}



function updateDeviceInfo(dSUID)
{
  apiCall({
    "method":"getProperty",
    "dSUID":dSUID,
    "query":{
      "x-p44-opStateLevel":null, "x-p44-opStateText":null
    }
  }).done(function(device) {
    var stateInfo = device['x-p44-opStateText'];
    if (stateInfo.length==0) {
      stateInfo = "OK";
    }
    $('#deviceStatusInfo').html(opStateSpan(device['x-p44-opStateLevel'], stateInfo));
    // poll every 10 sec again if still open
    if (deviceInfoIsOpen) {
      deviceInfoUpdaterTimeout = setTimeout(function() { updateDeviceInfo(dSUID); }, 2000);
    }
  });
}


function openRenameAddressable(dSUID)
{
  apiCall({
    "method":"getProperty",
    "dSUID":dSUID,
    "query":{
      "name":null
    }
  }).done(function(device) {
    $('#newAddressableName').val(device.name); // set current name
    $('#addressableNameApply').off("click");
    $('#addressableNameApply').on("click", function(event) { applyAddressableName(dSUID); });
    openDialog('#addressablenameedit');
  });
}


function applyAddressableName(dSUID)
{
  // get data
  devname = $("#newAddressableName").val().toString();
  // set new name
  closeDialogAndRefreshDevices(apiCall({
    "method":"setProperty",
    "dSUID":dSUID,
    "properties":{ "name":devname }
  }));
}



function createAndAssignNewZone(dSUID)
{
  closeDialog(function() {
    $("#newZoneName").val('');
    $('#newZoneApply').off("click");
    $('#newZoneApply').on("click", function(event) { applyNewZone(dSUID); });
    openDialog('#newzonecreate');
  });
}


function applyNewZone(dSUID)
{
  // get data
  newZN = $("#newZoneName").val().toString();
  // create new zone
  apiCall({
    "method":"setProperty",
    "dSUID":"root",
    "properties":{ "x-p44-localController":{ "zones": { "auto": { "name":newZN }}}}
  }).done(function(result) {
    var newZoneID = result[0].element;
    closeDialog(function() {
      // assign new zone to device now
      if (newZoneID!=undefined) {
        setDeviceZone(dSUID, newZoneID)
      }
    });
  }).fail(function(){
    closeDialog();
  });
}


function setDeviceZone(dSUID, zoneID)
{
  // set new zone
  apiCall({
    "method":"setProperty",
    "dSUID":dSUID,
    "properties":{ "zoneID":zoneID }
  }).done(function() {
    // reload modified device list
    refresh_devicelist();
  });
}


function editZones()
{
  refresh_zoneeditorlist().done(function() {
    openDialog('#zoneeditor');
  });
}


function refresh_zoneeditorlist()
{
  var dfd = $.Deferred();
  refresh_zonelist().done(function() {
    // table header
    var tableHtml =
      '<table data-role="table" id="zonesTable" class="ui-body-a ui-shadow table-stripe ui-responsive">' +
      '<thead>' +
       '<tr class="ui-bar-a">' +
         '<th class="namecell">Zone</th>' +
         '<th class="namecell">Devices</th>' +
         '<th class="actioncell"></th>' +
       '</tr>' +
      '</thead>' +
      '<tbody>';
    for (var zoneid in zonelist) {
      zone = zonelist[zoneid];
      tableHtml += '<tr>';
      tableHtml +=
        '<td class="namecell">' +
        '<abbr title="Rename Zone..."><a onclick="openRenameZone(\'' + zoneid.toString() + '\').done(function() { editZones(); });" data-mini="true" data-role="button" data-icon="edit" data-iconpos="notext" data-theme="a" data-inline="true">Rename</a></abbr> ' +
        zone.name + '</td>';
      tableHtml += '<td>' + zone.deviceCount.toString() + '</td>';
      tableHtml += '<td class="actioncell">';
      if (zoneid!=0 && zone.deviceCount==0) {
        tableHtml += '<abbr title="Delete Zone..."><a onclick="deleteZone(' + zoneid.toString() + ');" data-mini="true" data-role="button" data-icon="delete" data-theme="a" data-inline="true">Delete...</a></abbr> ';
      }
      tableHtml += '</td></tr>';
    }
    // finalisation of table
    tableHtml +=
      '</tbody>' +
      '</table>';
    $("#zoneeditorlist").html(tableHtml).trigger('create');
    dfd.resolve();
  }).fail(function(domain, code, message) {
    dfd.reject(domain, code, message);
  });
  return dfd.promise();
}


function openRenameZone(zoneid)
{
  var dfd = $.Deferred();
  var zone = zonelist[zoneid];
  $('#renamedZoneName').val(zone.name); // set current name
  $('#zoneRenameApply').off("click");
  $('#zoneRenameApply').on("click", function(event) {
    // get data
    newZN = $("#renamedZoneName").val().toString();
    // set new name
    apiCall({
      "method":"setProperty",
      "dSUID":"root",
      "properties":{ "x-p44-localController":{ "zones": { [zoneid]:{ "name":newZN }}}}
    }).always(function() {
      closeDialog(function() { dfd.resolve(); });
    });
  });
  $('#zoneRenameCancel').off("click");
  $('#zoneRenameCancel').on("click", function(event) {
    closeDialog(function() { dfd.resolve(); });
  });
  openDialog('#zonenameedit');
  return dfd.promise();
}


function deleteZone(zoneid)
{
  openDialog("#removeZoneConfirm", function() {
    $("#removeZoneNowButton").off('click.edit'); // remove previous one
    $("#removeZoneNowButton").on('click.edit', function () { deleteZoneNow(zoneid); });
  });
}


function deleteZoneNow(zoneid)
{
  // remove now
  apiCall({
    "method":"setProperty",
    "dSUID":"root",
    "properties":{
      "x-p44-localController":{ "zones": { [zoneid]:null }}
    }
  }).always(function() {
    closeDialog(function () {
      // back to editor
      editZones();
    });
  });
}





function setDeviceConfiguration(dSUID)
{
  // set new configuration
  // { "method":"setConfiguration", "dSUID": "xxx", "configurationId":"yyy" }
  apiCall({
    "method":"setConfiguration",
    "dSUID":dSUID,
    "configurationId": $("#configurationVariants_select").val()
  }).always(function() {
    closeDialog(function() {
      // reload modified device list
      refresh_devicelist();
    });
  });
}




function featureModeChannelHtml(channelID)
{
  var html =
    '<label>gradient features for brightness/hue/saturation</label>' +
    '<fieldset data-role="controlgroup" data-type="horizontal" data-mini="true">';
  for (var i=0; i<3; i++) {
    html +=
      '<select name="fs_' + channelID + '_' + i.toString() + '_curve" id="fs_' + channelID + '_' + i.toString() + '_curve">' +
      '  <option value="0">none</option>' +
      '  <option value="1">edge</option>' +
      '  <option value="2">linear</option>' +
      '  <option value="3">sin</option>' +
      '  <option value="4">cos</option>' +
      '  <option value="5">log</option>' +
      '  <option value="6">exp</option>' +
      '  <option value="9">inv edge</option>' +
      '  <option value="10">inv linear</option>' +
      '  <option value="11">inv sin</option>' +
      '  <option value="12">inv cos</option>' +
      '  <option value="13">inv log</option>' +
      '  <option value="14">inv exp</option>' +
      '</select>';
    html +=
      '<select name="fs_' + channelID + '_' + i.toString() + '_repeat" id="fs_' + channelID + '_' + i.toString() + '_repeat">' +
      '  <option value="0">once</option>' +
      '  <option value="16">cyclic</option>' +
      '  <option value="32">updown</option>' +
      '  <option value="48">open</option>' +
      '</select>';
  }
  html +=
    '</fieldset>';
  html +=
    '<label>global lightspot settings</label>' +
    '<fieldset data-role="controlgroup" data-type="horizontal" data-mini="true">' +
      '<select name="fs_' + channelID + '_direction" id="fs_' + channelID + '_direction">' +
      '  <option value="0">radial</option>' +
      '  <option value="1">linear</option>' +
      '</select>' +
      '<select name="fs_' + channelID + '_clip" id="fs_' + channelID + '_clip">' +
      '  <option value="0">clip to size</option>' +
      '  <option value="1">fill area</option>' +
      '</select>' +
      '<select name="fs_' + channelID + '_center" id="fs_' + channelID + '_center">' +
      '  <option value="0">origin centered in content</option>' +
      '  <option value="1">origin bottom left</option>' +
      '</select>' +
    '</fieldset>';
  return html;
}


function getFeatureMode(channelID)
{
  var mode = 0;
  for (var i=0; i<3; i++) {
    var pfx = '#fs_' + channelID + '_' + i.toString();
    mode += $(pfx + '_curve').val() << (8*i);
    mode += $(pfx + '_repeat').val() << (8*i);
  }
  mode += $('#fs_' + channelID + '_direction').val()>0 ? 1<<24 : 0;
  mode += $('#fs_' + channelID + '_clip').val()>0 ? 1<<25 : 0;
  mode += $('#fs_' + channelID + '_center').val()>0 ? 1<<26 : 0;
  return mode;
}


function featureModeChannelSetValues(dSUID, channelID, value)
{
  //console.log("mode = " + value.toString(16));
  for (var i=0; i<3; i++) {
    var pfx = '#fs_' + channelID + '_' + i.toString();
    $(pfx + '_curve').off("change");
    $(pfx + '_curve').val(value & 0x0F).selectmenu("refresh");
    $(pfx + '_curve').on("change", function(event, ui) { changedFeatureChannelSelect(event, dSUID, channelID)});
    $(pfx + '_repeat').off("change");
    $(pfx + '_repeat').val(value & 0xF0).selectmenu("refresh");
    $(pfx + '_repeat').on("change", function(event, ui) { changedFeatureChannelSelect(event, dSUID, channelID)});
  }
  // direction
  $('#fs_' + channelID + '_direction').off("change");
  $('#fs_' + channelID + '_direction').val((value & 0x01000000)==0 ? 0 : 1).selectmenu("refresh");
  $('#fs_' + channelID + '_direction').on("change", function(event, ui) { changedFeatureChannelSelect(event, dSUID, channelID)});
  // clip
  $('#fs_' + channelID + '_clip').off("change");
  $('#fs_' + channelID + '_clip').val((value & 0x02000000)==0 ? 0 : 1).selectmenu("refresh");
  $('#fs_' + channelID + '_clip').on("change", function(event, ui) { changedFeatureChannelSelect(event, dSUID, channelID)});
  // center
  $('#fs_' + channelID + '_center').off("change");
  $('#fs_' + channelID + '_center').val((value & 0x04000000)==0 ? 0 : 1).selectmenu("refresh");
  $('#fs_' + channelID + '_center').on("change", function(event, ui) { changedFeatureChannelSelect(event, dSUID, channelID)});
}


function changedFeatureChannelSelect(event, dSUID, channelID)
{
  var el = $('#'+event.currentTarget.id);
  el.selectmenu("refresh");
  var mode = getFeatureMode(channelID);
  //console.log("mode = " + mode.toString(16));
  var changequery = {
    "method":"setProperty",
    "dSUID":dSUID,
    "properties":{
      "channelStates": { }
    }
  };
  changequery.properties.channelStates[channelID.toString()] = { "value": mode };
  apiCall(changequery, 1000);
}


function updateCouplingModeDeps(mode)
{
  showIf(mode!=0, '#channelCouplingOpts')
  showIf(mode>=200, '#couplingScriptEditorLink')
}


function channelCouplingSelected(dSUID)
{
  var mode = $('#channelCouplingSelector').val();
  updateCouplingModeDeps(mode)
  apiCall({ method:"setProperty", dSUID:dSUID, properties:{ outputSettings: { "x-p44-couplingMode":mode }}})
}


function channelCouplingParamChanged(dSUID)
{
  var param = $('#channelCouplingParam').val();
  apiCall({ method:"setProperty", dSUID:dSUID, properties:{ outputSettings: { "x-p44-couplingParam":param }}})
}



function channelValueChangeAttachEvent(dSUID, channelID)
{
  $('#channel_value_' + channelID).off("change");
  $('#channel_value_' + channelID).on("change", function(event, ui) { changedChannelValue(event, dSUID, channelID)});
}


var allChannelsRefreshTimeout = 0;
var channelsDialogIsOpen = false;

function openDeviceChannels(event, hiddenFeatures)
{
  var dSUID = event.currentTarget.getAttribute('dSUID');
  hiddenFeatures ||= event.shiftKey;
  channelsDialogIsOpen = true;
  // stop updatding inputs when popup closes!
  $("#channelControl").off('popupafterclose.channelctrl');
  $("#channelControl").on('popupafterclose.channelctrl', function() {
    clearTimeout(allChannelsRefreshTimeout);
    channelsDialogIsOpen = false;
  });
  // query channel details
  apiCall({
    "method":"getProperty",
    "dSUID":dSUID,
    "query":{
      "deviceIconName":null, "dSUID":null, "name":null, "model":null,
      "outputSettings":null,
      "channelDescriptions":null,
      "channelStates":null
    }
  }).done(function(device) {
    // device info
    $('#channelInfoIcon').html('<img src="/icons/icon16/' + device.deviceIconName + '.png" />');
    $('#channelInfoTitle').html(device.name + ': Outputchannels');
    // channel sliders
    var channelsHTML = '';
    for (var channelID in device.channelDescriptions) {
      var channel = device.channelDescriptions[channelID]
      var channelType = channel.channelType
      var max = channel.max
      var min = channel.min
      var resolution = channel.resolution
      var step = resolution>=(max-min)/200  ? resolution : 0.5 // we cannot have more than 200 steps
      var value = roundToResolution(device.channelStates[channelID].value, resolution)
      if (channel.channelType==200) {
        // feature channel, just show a text field
        channelsHTML += featureModeChannelHtml(channelID)
      }
      else
      {
        channelsHTML +=
          '<label for="channel_value_' + channelID + '">' +
          '<abbr title="If checked, channel value will be loaded/stored from scene selected below (Note: clicking immediately changes stored scene flags!)"><input data-role="none" class="channelDoCareCbx hidden" type="checkbox" onclick="sceneChannelFlagChange(\'' + dSUID + '\',\'' + channelID + '\')" id="channel_docare_' + channelID + '"></abbr>' + channel.name + '</label>' +
          '<input type="range" class="slider_' + channelID + '" data-highlight="' + (channelType==1 || channelType==2 || channelType==4 ? 'false' : 'true') + '" name="channel_value_' + channelID + '" id="channel_value_' + channelID + '"' +
          ' min="' + min.toString() +'" max="' + max.toString() + '" step="' + step.toString() + '" value="' + value.toString() + '"' +
          ' />' + "\n";
      }
    }
    if (hiddenFeatures) {
      // channel coupling
      if (device.outputSettings['x-p44-couplingMode']!==undefined) {
        channelsHTML +=
          '<p>&nbsp;</p><label for="channelCouplingSelector">Channel coupling:</label>' +
          '<select name="channelCouplingSelector" id="channelCouplingSelector" data-mini="true" onchange="channelCouplingSelected(\''  + dSUID + '\')">' +
          '  <option value="0">none</option>' +
          '  <option title="color temperature decreases with brightness" value="1">glow dim (ct follows brightness)</option>' +
          '  <option value="254">custom scripted brightness dependency</option>' +
          '  <option value="255">custom scripted dependencies</option>' +
          '</select>' +
          '<div style="display:none" id="channelCouplingOpts">' +
            '<label for="channelCouplingParam">Parameter for channel coupling</label>' +
            '<input type="text" autocomplete="off" name="channelCouplingParam" onblur="channelCouplingParamChanged(\''  + dSUID + '\')" id="channelCouplingParam" value="" >' +
            '<a style="display:none" onmousedown="return false;" onclick="closeDialog(); return true;" id="couplingScriptEditorLink" href="" target="_blank">Edit script in p44script IDE</a>' +
          '</div>'
      }
      // scene editing directly from output
      channelsHTML +=
        '<p>&nbsp;</p><label for="sceneEditSelector">Scene:</label>' +
        '<select name="sceneEditSelector" id="sceneEditSelector" data-mini="true" onchange="sceneSelected(\''  + dSUID + '\')">' +
        '  <option value="-1">-</option>'
      sceneslist.forEach(function(o) {
        channelsHTML += ` <option value="${o.sceneNo}">${escapehtml(o.name)}</option>`
      });
      channelsHTML +=
        '</select>' +
        '<button type="button" onclick="channelSceneOp(\'' + dSUID + '\',\'call\', event);" id="callSceneButton" data-mini="true" data-inline="true" data-icon="action">Call</button>' +
        '<button type="button" onclick="channelSceneOp(\'' + dSUID + '\',\'stop\', event);" id="stopSceneButton" data-mini="true" data-inline="true" data-icon="forbidden">Stop</button>' +
        '<button type="button" onclick="channelSceneOp(\'' + dSUID + '\',\'save\', event);" id="saveSceneButton" data-mini="true" data-inline="true" data-icon="check">Save</button>' +
        '<button type="button" onclick="channelSceneOp(\'' + dSUID + '\',\'edit\', event);" id="editSceneButton" data-mini="true" data-inline="true" data-icon="edit">Edit...</button>' +
        "\n";
    }
    $('#channelList').html(channelsHTML).trigger('create')
    if (hiddenFeatures && device.outputSettings['x-p44-couplingMode']!==undefined) {
      $('#channelCouplingSelector').val(device.outputSettings['x-p44-couplingMode']).selectmenu('refresh')
      updateCouplingModeDeps(device.outputSettings['x-p44-couplingMode'])
      $('#channelCouplingParam').val(device.outputSettings['x-p44-couplingParam'])
      activateIDElink(
        '#couplingScriptEditorLink',
        function() { // ID getter
          var dfd = $.Deferred();
          var scriptid = device['x-p44-couplingScriptId']
          if (scriptid) {
            // we have it already
            dfd.resolve(scriptid)
          }
          else {
            // we need to get it because the script was not active when the dialog was loaded
            apiCall(
              { method:"getProperty", dSUID:dSUID, query:{ outputSettings: { "x-p44-couplingScriptId":null } }
            }).done(function(result) {
              dfd.resolve(result.outputSettings['x-p44-couplingScriptId'])
            })
          }
          return dfd
        },
        function() { // existing script saver)
          if (device.outputSettings['x-p44-couplingScript']=="") {
            // store a STX to activate the script
            return apiCall({
              method:"setProperty",
              dSUID:dSUID,
              properties:{ outputSettings: { "x-p44-couplingScript":"\x02" } }
            })
          }
          else {
            var dfd = $.Deferred();
            dfd.resolve()
            return dfd;
          }
        }
      )
    }
    for (var channelID in device.channelDescriptions) {
      if (device.channelDescriptions[channelID].channelType==200) {
        featureModeChannelSetValues(device.dSUID, channelID, device.channelStates[channelID].value);
      }
      else
      {
        channelValueChangeAttachEvent(device.dSUID, channelID);
      }
    }
    // constant channel refresh
    allChannelsRefreshTimeout = setTimeout(function() { updateChannelValues(dSUID) }, 5000);
    openDialog('#channelControl');
  });
}





function sceneSelected(dSUID)
{
  var sceneNo = parseInt($('#sceneEditSelector').val());
  if (sceneNo<0) $('.channelDoCareCbx').addClass('hidden');
  else {
    $('.channelDoCareCbx').removeClass('hidden');
    // update do(nt)care checkboxes from actual scene
    apiCall({
      "method":"getProperty",
      "dSUID":dSUID,
      "query":{
        "scenes": { [sceneNo]: { "channels": null } }
      }
    }).done(function(res) {
      let sc = res.scenes[sceneNo].channels;
      for (let sid in sc) {
        let ch = sc[sid]
        // Note: jquery attr() does not work correctly on this data-role="none" checkbox,
        //   so we're using direct DOM element methods. Newer jquery would have prop(), but 1.1 does not.
        let cb = $('#channel_docare_'+sid)
        if (cb) {
          cb.get(0).removeAttribute("checked")
          if (!ch.dontCare) cb.get(0).setAttribute("checked", "true")
        }
      }
    });
  }
}


function sceneChannelFlagChange(dSUID, channelID)
{
  var sceneNo = parseInt($('#sceneEditSelector').val());
  alertError(apiCall({
    "method":"setProperty",
    "dSUID":dSUID,
    "properties":{
      "scenes": { [sceneNo]: { "channels": { [channelID]: { "dontCare": !$('#channel_docare_'+channelID).is(':checked') } } } }
    }
  }))
}


function channelSceneOp(dSUID, op, event)
{
  var target = getTarget(event)
  var sceneNo = parseInt($('#sceneEditSelector').val());
  if (sceneNo>=0) {
    if (op=='stop') {
      stopSceneActions(dSUID, event);
      setTimeout(function() { updateChannelValues(dSUID); }, 800);
    }
    else if (op=='call') {
      // call scene
      apiCall({
        "notification":"callScene",
        "dSUID":dSUID,
        "scene":sceneNo,
        "force":true
      }).always(function() {
        buttonFeedback(target, 'orange');
        setTimeout(function() { updateChannelValues(dSUID); }, 800);
      });
    }
    else if (op=='save') {
      // save scene
      apiCall({
        "notification":"saveScene",
        "dSUID":dSUID,
        "scene":sceneNo
      }).done(function() {
        buttonFeedback(target, 'green');
      });
    }
    else if (op=='edit') {
      editScene(sceneNo, dSUID, $('#sceneEditSelector option:selected').text());
    }
  }
}


function stopSceneActionsAndRefreshLights(dSUID, event)
{
  stopSceneActions(dSUID, event);
  setTimeout(function() { refresh_lightslist() }, 800);
}



function stopSceneActions(dSUID, event)
{
  var target = getTarget(event)
  apiCall({
    "method": "x-p44-stopSceneActions",
    "dSUID":dSUID
  }).done(function() {
    buttonFeedback(target, 'red')
  });
}



function openSingleDevice(event)
{
  var dSUID = event.currentTarget.getAttribute('dSUID');
  // query single device details
  apiCall({
    "method":"getProperty",
    "dSUID":dSUID,
    "query":{
      "deviceIconName":null, "dSUID":null, "name":null, "model":null,
      "deviceActionDescriptions":null,
      "standardActions":null,
      "customActions":null,
      "deviceStateDescriptions":null,
      "devicePropertyDescriptions":null
    }
  }).done(function(device) {
    // device info
    $('#singleDeviceIcon').html('<img src="/icons/icon16/' + device.deviceIconName + '.png" />');
    $('#singleDeviceTitle').html(device.name + ' Controls');
    // action selector
    var actionsHTML = '<option value="0">-- Select action--</option>';
    for (var actionId in device.deviceActionDescriptions) {
      var action = device.deviceActionDescriptions[actionId];
      var description = action.description;
      actionsHTML +=
        '<option value="' + actionId + '">' + escapehtml(description) + '</option>';
    }
    for (var actionId in device.standardActions) {
      var stdaction = device.standardActions[actionId];
      var description = stdaction.description;
      actionsHTML +=
        '<option value="' + actionId + '">' + escapehtml(description) + '</option>';
    }
    for (var actionId in device.customActions) {
      var custaction = device.customActions[actionId];
      var description = custaction.description;
      actionsHTML +=
        '<option value="' + actionId + '">' + escapehtml(description) + '</option>';
    }
    $('#singleDevice_action_select').html(actionsHTML).trigger('create');
    $('#singleDevice_invoke_button').off("click");
    $('#singleDevice_invoke_button').on("click", function(event) { invokeDeviceAction(dSUID); });
    openDialog("#singleDevice");
  });
}


function refresh_action_deps()
{
  // NOP for now
  //alert("action changed");
}


function invokeDeviceAction(dSUID)
{
  var actionId = $('#singleDevice_action_select').val();
  //alert("will call action " + actionId);
  if (actionId!=='0') {
    // invoke the action
    // { "method":"invokeDeviceAction", "dSUID": "xxx", "params":{ "id":"std.lattemacchiato" } }
    apiCall({
      "method":"invokeDeviceAction",
      "dSUID":dSUID,
      "id":actionId,
      "params": {
      }
    }).done(function() {
      alert("action '" + actionId + "'invoked");
    });
  }
}


var channelValueUpdaterTimeout = 0;
var channelValueSetRetryTimeout = 0;
var channelValuesUpdating = false;
var fastChangeHoldoff = false;

function changedChannelValue(event, dSUID, channelID)
{
  clearTimeout(channelValueUpdaterTimeout);
  clearTimeout(allChannelsRefreshTimeout); // also pause all channels updater
  if (!channelValuesUpdating) {
    var value = $('#channel_value_' + channelID.toString()).val();
    if (fastChangeHoldoff) {
      // reset possibly pending retry of an earlier value
      clearTimeout(channelValueSetRetryTimeout);
      // schedule retry later
      channelValueSetRetryTimeout = setTimeout(function() { changedChannelValue(event, dSUID, channelID); }, 50);
    }
    else {
      //console.log('channelID=' + channelID.toString() + ' value=' + value.toString());
      var changequery = {
        "method":"setProperty",
        "dSUID":dSUID,
        "properties":{
          "channelStates": {
            [channelID.toString()]: { "value": value }
          }
        }
      };
      fastChangeHoldoff = true; // prevent another change too soon
      apiCall(changequery, 1000).done(function() {
        // prevent another change too soon
        setTimeout(function() { fastChangeHoldoff = false; }, 100);
        // after a while, read back other channels to see dependencies
        channelValueUpdaterTimeout = setTimeout(function() { updateChannelValues(dSUID, channelID); }, 1000);
      }).fail(function() {
        fastChangeHoldoff = false;
      });
    }
  }
}


function updateChannelValues(dSUID, exceptChannelID)
{
  if (!channelValuesUpdating) {
    channelValuesUpdating = true;
    apiCall({
      "method":"getProperty",
      "dSUID":dSUID,
      "query":{
        "channelStates":null
      }
    }).done(function(device) {
      //console.log('Updating channels');
      for (var channelID in device.channelStates) {
        if (exceptChannelID===undefined || channelID!=exceptChannelID) {
          var value = roundToFracDigits(device.channelStates[channelID].value,2)
          var inp = $('#channel_value_' + channelID.toString())
          if (!inp.hasClass('ui-focus')) { // prevent updating numeric input being edited
            inp.val(value);
            if (inp.slider!==undefined) inp.slider("refresh");
          }
        }
      }
      channelValuesUpdating = false;
        // after success: next refresh in 5 secs
      if (channelsDialogIsOpen) {
        allChannelsRefreshTimeout = setTimeout(function() { updateChannelValues(dSUID) }, 5000);
      }
      }).fail(function() {
      channelValuesUpdating = false;
        // after failure: wait with next refresh for 20 secs
      if (channelsDialogIsOpen) {
        allChannelsRefreshTimeout = setTimeout(function() { updateChannelValues(dSUID) }, 20000);
      }
      });
  }
}



function refresh_buttontype_deps()
{
  showIf($('#buttonFuncSelect').val()>=1000 && $('#buttonModeSelect').val()!=2, '#actionButtonControls') // action function, makes no sense for dim-only
  showIf($('#buttonModeSelect').val()!=1 && $('#buttonFuncSelect').val()!=15, '#dimbuttoncontrols') // if not click-only or custom
  showIf($('#buttonFuncSelect').val()!=15, '#buttonGroupControls') // only if not custom
}



function openButtonSettings(event)
{
  var dSUID = event.currentTarget.getAttribute('dSUID')
  // query device details
  apiCall({
    method:"getProperty",
    dSUID:dSUID,
    query:{
      name:null, deviceIconName:null,
      buttonInputDescriptions:null,
      buttonInputSettings:null
    }
  }, 3000).done(function(device) {
    // global
    $('#buttonSettingsIcon').html('<img src="/icons/icon16/' + device.deviceIconName + '.png" />')
    $('#buttonSettingsTitle').html(device.name + ': Button Settings')
    // use info from first button (later save to all)
    var bk = Object.keys(device.buttonInputDescriptions)[0]
    var desc = device.buttonInputDescriptions[bk]
    var settings = device.buttonInputSettings[bk]
    // - scenes selector
    $('#buttonSceneSelect').empty()
      sceneslist.forEach(function(o) {
      $('#buttonSceneSelect').append(new Option(o.name, o.sceneNo));
    });
    $('#buttonSceneSelect').val(settings['x-p44-buttonActionId']).selectmenu("refresh");
    // - button group, only visible if more than one kind of devices available at all
    var gchtml = groupselector('buttonGroupSelect', false);
    var group = settings.group;
    if (gchtml.length>0) {
      $('#buttonGroupControls').html(
        '<label for="buttonGroupSelect">Button affects:</label>' +
        gchtml
      ).trigger('create');
      $('#buttonGroupSelect').val(group).selectmenu("refresh");
    }
    else {
      $('#buttonGroupControls').html(''); // no group changer
    }
    // - button function
    var actionmode = settings['x-p44-buttonActionMode']
    var funcval = actionmode==255 ? settings.function : 1000+actionmode
    $('#buttonFuncSelect').val(funcval).selectmenu("refresh")
    // - button (statemachine) mode
    $('#buttonModeSelect').val(settings['x-p44-stateMachineMode']).selectmenu("refresh")
    // - button channel
    $('#buttonChannelSelect').val(settings.channel).selectmenu("refresh")
    // save handler
    $('#buttonSettingsApply').off("click.apply")
    $('#buttonSettingsApply').on("click.apply", function(event) {
      if (gchtml.length>0 && $('#buttonFuncSelect').val()!=15) {
        group = $("#buttonGroupSelect").val();
      }
      // update settings in all button elements
      var buttonfunction = $('#buttonFuncSelect').val()
      var settings = {
        group:group,
        "x-p44-stateMachineMode":$('#buttonModeSelect').val(),
        channel:$('#buttonChannelSelect').val()
      }
      if (buttonfunction<1000) {
        settings['x-p44-buttonActionMode'] = 255
        settings.function = buttonfunction
      }
      else {
        settings['x-p44-buttonActionMode'] = buttonfunction-1000
        settings['x-p44-buttonActionId'] = $('#buttonSceneSelect').val()
      }
      alertError(apiCall({
        method:"setProperty",
        dSUID:dSUID,
        properties: { buttonInputSettings:{ "*": settings }}
      })).done(function() {
        closeDialog()
      });
    });
    refresh_buttontype_deps()
    openDialog("#buttonSettings")
  });
}


function openSensorSettings(event)
{
  var dSUID = event.currentTarget.getAttribute('dSUID');
  var bk = event.currentTarget.getAttribute('sensorID');
  // query device details
  apiCall({
    method:"getProperty",
    dSUID:dSUID,
    query:{
      name:null, deviceIconName:null,
      sensorDescriptions:{ [bk]:null },
      sensorSettings:{ [bk]:null }
    }
  }, 3000).done(function(device) {
    // global
    $('#sensorSettingsIcon').html('<img src="/icons/icon16/' + device.deviceIconName + '.png" />');
    $('#sensorSettingsTitle').html(device.name + ': Sensor Settings');
    // use info from first button (later save to all)
    var desc = device.sensorDescriptions[bk];
    var settings = device.sensorSettings[bk];
    // - sensor group, only visible if more than one kind of devices available at all
    var gchtml = groupselector('sensorGroupSelect', false);
    var group = settings.group;
    if (gchtml.length>0) {
      $('#sensorGroupControls').html(
        '<label for="sensorGroupSelect">Sensor affects:</label>' +
        gchtml
      ).trigger('create');
      $('#sensorGroupSelect').val(group).selectmenu("refresh");
    }
    else {
      $('#sensorGroupControls').html(''); // no group changer
    }
    showIf(desc.sensorUsage==3, '#userSensorControls');
    // - sensor function
    $('#sensorFuncSelect').val(settings.function).selectmenu("refresh");
    // - sensor channel
    $('#sensorChannelSelect').val(settings.channel).selectmenu("refresh");
    // save handler
    $('#sensorSettingsApply').off("click.apply")
    $('#sensorSettingsApply').on("click.apply", function(event) {
      if (gchtml.length>0) {
        group = $("#sensorGroupSelect").val();
      }
      // update settings in all sensor elements
      alertError(apiCall({
        method:"setProperty",
        dSUID:dSUID,
        properties:{
          sensorSettings:{ [bk]: {
            group:group,
            function:$('#sensorFuncSelect').val(),
            channel:$('#sensorChannelSelect').val()
          }}
        }
      })).done(function() {
        closeDialog();
      });
    });
    openDialog("#sensorSettings");
  });
}



var inputValueUpdaterTimeout = 0;
var deviceInputsIsOpen = false;

function openDeviceInputs(event)
{
  var dSUID = event.currentTarget.getAttribute('dSUID');
  deviceInputsIsOpen = true;
  // stop updatding inputs when popup closes!
  $("#inputInfo").off('popupafterclose.devinp');
  $("#inputInfo").on('popupafterclose.devinp', function() {
    clearTimeout(inputValueUpdaterTimeout);
    deviceInputsIsOpen = false;
  });
  // query input details
  apiCall({
    "method":"getProperty",
    "dSUID":dSUID,
    "query":{
      "deviceIconName":null, "dSUID":null, "name":null, "model":null,
      "sensorDescriptions":null,
      "binaryInputDescriptions":null,
      "deviceStateDescriptions":null,
      "devicePropertyDescriptions":null,
    }
  }).done(function(device) {
    // device info
    $('#inputInfoIcon').html('<img src="/icons/icon16/' + device.deviceIconName + '.png" />');
    $('#inputInfoTitle').html(device.name + ': Sensors and Inputs');
    // input info
    var inputsHTML = '<table class="inputsInfoTable"><thead><tr><th class="inputsTableDesc">Description</th><th class="inputsTableValue">Value</th><th class="inputsTableUnit">Unit</th><th class="inputsTableAge">Last&nbsp;updated</th><th class="inputsTableError">Status</th><th>&nbsp;</th></tr><tbody>';
    for (var sensorId in device.sensorDescriptions) {
      var sensor = device.sensorDescriptions[sensorId];
      inputsHTML += '<tr><td class="inputsTableDesc">';
      if (sensor['x-p44-rrdFile']!=undefined) {
        // sensor is being logged into a RRD file
        inputsHTML += '<a target="_blank" href="/rrd.html#' + sensor['x-p44-rrdFile'] + '">' + sensor.name + '</a>';
      }
      else {
        // just text
        inputsHTML += sensor.name;
      }
      inputsHTML += '</td><td class="inputsTableValue" id="sensor_value_' + sensorId.toString() + '"></td><td class="inputsTableUnit">' + sensor.symbol + '</td><td class="inputsTableAge" id="sensor_age_' + sensorId.toString() + '"></td><td class="inputsTableError" id="sensor_error_' + sensorId.toString() + '"></td>';
      if (sensor.sensorUsage==3) {
        inputsHTML +=
          '<td><abbr title="Edit Sensor Settings..."><a dSUID="' + dSUID + '" sensorID="' + sensorId + '" onclick="openSensorSettings(event);" data-mini="true" data-role="button" data-icon="grid" data-iconpos="notext" data-inline="true">Sensor</a></abbr></td>'
      }
      else
      {
        inputsHTML += '<td>&nbsp;</td>';
      }
      inputsHTML += '</tr>';
    }
    for (var bininpId in device.binaryInputDescriptions) {
      var bininp = device.binaryInputDescriptions[bininpId];
      inputsHTML += '<tr><td class="inputsTableDesc">' + bininp.name + '</td><td class="inputsTableValue" id="input_value_' + bininpId.toString() + '"></td><td class="inputsTableUnit">&nbsp;</td><td class="inputsTableAge" id="input_age_' + bininpId.toString() + '"></td><td class="inputsTableError" id="input_error_' + bininpId.toString() + '"></td><td>&nbsp;</td></tr>';
    }
    for (var stateName in device.deviceStateDescriptions) {
      var state = device.deviceStateDescriptions[stateName];
      inputsHTML += '<tr><td class="inputsTableDesc">' + stateName + '</td><td class="inputsTableValue" id="state_value_' + stateName + '"></td><td class="inputsTableUnit">' + (state.value.symbol==undefined ? '' : state.value.symbol) + '</td><td class="inputsTableAge" id="state_age_' + stateName + '"></td><td class="inputsTableError">n/a</td><td>&nbsp;</td></tr>';
    }
    for (var propName in device.devicePropertyDescriptions) {
      var prop = device.devicePropertyDescriptions[propName];
      inputsHTML += '<tr><td class="inputsTableDesc">' + propName + '</td><td class="inputsTableValue" id="prop_value_' + propName + '"></td><td class="inputsTableUnit">' + (prop.symbol==undefined ? '' : prop.symbol) + '</td><td class="inputsTableAge">n/a</td><td class="inputsTableError">n/a</td><td>&nbsp;</td></tr>';
    }
    inputsHTML += '</tbody></table/>';
    $('#inputList').html(inputsHTML).trigger('create');
    openDialog("#inputInfo");
    updateDeviceInputs(dSUID);
  });
}


function updateDeviceInputs(dSUID)
{
  // query input ages and values
  apiCall({
    "method":"getProperty",
    "dSUID":dSUID,
    "query":{
      "deviceIconName":null, "dSUID":null, "name":null, "model":null,
      "sensorStates":null,
      "sensorDescriptions":{ "": { "resolution":null, "aliveSignInterval":null }},
      "binaryInputStates":null,
      "deviceStates":null,
      "deviceProperties":null,
    }
  }).done(function(device) {
    for (var sensorId in device.sensorStates) {
      var sensor = device.sensorStates[sensorId];
      if (sensor.age!=null) {
        $('#sensor_age_' + sensorId.toString()).html(formattedAge(sensor.age, 3) + ' ago');
        fracDigits = fracDigitsForResolution(device.sensorDescriptions[sensorId].resolution)
        $('#sensor_value_' + sensorId.toString()).html(sensor.value.toFixed(fracDigits).toString())
      }
      else {
        $('#sensor_age_' + sensorId.toString()).html('');
        $('#sensor_value_' + sensorId.toString()).html('');
      }
      $('#sensor_error_' + sensorId.toString()).html(sensor.error==0 ? 'OK' : 'Error '+sensor.error.toString());
    }
    for (var bininpId in device.binaryInputStates) {
      var bininp = device.binaryInputStates[bininpId];
      if (bininp.age!=null) {
        $('#input_age_' + bininpId.toString()).html(formattedAge(bininp.age, 3) + ' ago');
        if (bininp.extendedValue!=undefined) {
          // multi-value
          $('#input_value_' + bininpId.toString()).html(bininp.extendedValue.toString());
        }
        else {
          // binary
          $('#input_value_' + bininpId.toString()).html(bininp.value.toString());
        }
      }
      else {
        $('#input_age_' + bininpId.toString()).html('');
        $('#input_value_' + bininpId.toString()).html('');
      }
      $('#input_error_' + bininpId.toString()).html(bininp.error==0 ? 'OK' : 'Error '+bininp.error.toString());
    }
    for (var stateName in device.deviceStates) {
      var state = device.deviceStates[stateName];
      if (state.changed!=null) {
        $('#state_age_' + stateName).html(formattedAge(state.changed, 3) + ' ago');
        $('#state_value_' + stateName).html(state.value.toString());
      }
      else {
        $('#state_age_' + stateName).html('');
        $('#state_value_' + stateName).html('');
      }
    }
    for (var propName in device.deviceProperties) {
      var prop = device.deviceProperties[propName];
      if (prop!=null) {
        $('#prop_value_' + propName).html(prop.toString());
      }
      else {
        $('#prop_value_' + propName).html('');
      }
    }
    // poll every 10 sec again
    if (deviceInputsIsOpen) {
      inputValueUpdaterTimeout = setTimeout(function() { updateDeviceInputs(dSUID); }, 2000);
    }
  });
}



var zonelist = {};
var grouplist = {};


function refresh_zones_and_groups()
{
  var dfd = $.Deferred();
  refresh_zonelist().always(function() {
    refresh_grouplist().always(function() {
      dfd.resolve();
    });
  });
  return dfd.promise();
}


function refresh_zonelist()
{
  var dfd = $.Deferred();
  apiCall({
    "method":"getProperty",
    "dSUID":"root",
    "query":{
      "x-p44-localController":{ "zones":null }
    }
  }, 15000).done(function(result) {
    var localcontroller = result["x-p44-localController"];
    zonelist = localcontroller.zones;
    dfd.resolve(zonelist);
  }).fail(function(domain, code, message) {
    dfd.reject(domain, code, message);
  });
  return dfd.promise();
}


function refresh_grouplist()
{
  var dfd = $.Deferred();
  apiCall({
    "method":"x-p44-queryGroups",
    "dSUID":"root"
  }, 15000).done(function(result) {
    grouplist = result;
    dfd.resolve(grouplist);
  }).fail(function(domain, code, message) {
    dfd.reject(domain, code, message);
  });
  return dfd.promise();
}



function zonename(zoneid)
{
  var zone = zonelist[zoneid];
  var zn;
  if (zone!=undefined) {
    zn = zone.name;
  }
  else {
    zn = '#' + zoneid.toString();
  }
  return zn;
}


function zoneselectoroptions(selectedZoneId, withEdit)
{
  var optionshtml = '';
  for (var zoneid in zonelist) {
    zone = zonelist[zoneid];
    optionshtml += '<option value="' + zoneid + '"' + (zoneid==selectedZoneId ? " selected " : "") + '>' + zone.name + '</option>';
  }
  if (withEdit) {
    optionshtml += '<option value="-1">New Zone...</option>';
    optionshtml += '<option disabled><hr/></option>';
    optionshtml += '<option value="-2">Edit Zones...</option>';
  }
  return optionshtml;
}


function zoneselector(id, selectedZoneId, withEdit)
{
  var selectorhtml =
    '<select id="' + id + '">';
  selectorhtml += zoneselectoroptions(selectedZoneId, withEdit);
  selectorhtml += '</select>';
  return selectorhtml;
}



function groupselectoroptions(selectedGroupNo, always)
{
  var optionshtml = ''
  var numgroups = 0
  var foundselected = false
  for (var groupNo in grouplist) {
    group = grouplist[groupNo]
    numgroups++
    var selected = groupNo==selectedGroupNo
    optionshtml += '<option value="' + groupNo + '"' + (selected ? " selected " : "") + '>' + group.name + ' ' + group.symbol + '</option>'
    if (selected) foundselected = true
  }
  return (always===true || numgroups>1 || !foundselected) ? optionshtml : ""
}


function groupselector(id, selectedGroupNo)
{
  var opts = groupselectoroptions(selectedGroupNo, false);
  var selectorhtml = "";
  if (opts.length>0) {
    selectorhtml = '<select id="' + id + '" data-mini="true">';
    selectorhtml += opts;
    selectorhtml += '</select>';
  }
  return selectorhtml;
}



var deviceSortCriteria = 'name';

function devicelist_sortby(criteria)
{
  deviceSortCriteria = criteria;
  refresh_devicelist();
}

function devicelist_sortedByClass(criteria)
{
  if (criteria!=deviceSortCriteria) return "";
  return " sorted";
}


function deviceCompare(a,b)
{
  a1 = a.device[deviceSortCriteria];
  b1 = b.device[deviceSortCriteria];
  if (a1==undefined) {
    if (a1===b1) return 0;
    return -1;
  }
  else if (b1==undefined) {
    return 1;
  }
  else {
    var ret = a1.toString().localeCompare(b1.toString(), 'en', { 'numeric':true });
    if (ret==0 && deviceSortCriteria!='name') {
      ret = a.device.name.localeCompare(b.device.name, 'en');
    }
    return ret;
  }
}


function doRefreshDeviceList(event, hiddenFeatures)
{
  if (hiddenFeatures==2) {
    if (event.shiftKey) {
      // show vdchost properties in separate page
      openProperties("root")
      return
    }
    // hidden direct link to tweak.html
    openTweak()
    return
  }
  refresh_devicelist()
}



var deviceListRefreshTimeout = 0

function refresh_devices_statusinfo()
{
  var dfd = $.Deferred();
  apiCall({
    "method": "getProperty",
    "dSUID": "root",
    "query": {
      "x-p44-vdcs": { "": { "x-p44-devices": { "": {
        "dSUID": null, "active": null, "x-p44-opStateText": null, "x-p44-opStateLevel": null, "x-p44-statusText": null
      }}}}
    }
  }, 3000).done(function(result) {
    var vdcs = result["x-p44-vdcs"]
    for (var k in vdcs) {
      if (!vdcs.hasOwnProperty(k)) continue
      var devices = vdcs[k]["x-p44-devices"]
      for (var k2 in devices) {
        if (!devices.hasOwnProperty(k2)) continue
        var device = devices[k2];
        // update cells
        $('#'+device.dSUID+'_status').html(opStateSpan(device['x-p44-opStateLevel'], device['x-p44-opStateText'], device.active));
        $('#'+device.dSUID+'_info').html(device['x-p44-statusText']);
      }
    }
    dfd.resolve()
  }).fail(function(domain, code, message) {
    dfd.reject(domain, code, message)
  })
  return dfd.promise()
}


function schedule_refresh_devices_statusinfo(interval, standardinterval)
{
  if (standardinterval==undefined) standardinterval = interval
  clearTimeout(deviceListRefreshTimeout); // cancel previously scheduled refresh
  if (interval>0) {
    deviceListRefreshTimeout = setTimeout(function() {
      refresh_devices_statusinfo().done(function() {
        schedule_refresh_devices_statusinfo(standardinterval, standardinterval)
      }).fail(function() {
        console.log("interval refresh of device status failed, try next in 10*interval time")
        schedule_refresh_devices_statusinfo(10*standardinterval, standardinterval)
      })
    }, interval);
  }
}



function refresh_devicelist(focusDSUID)
{
  schedule_refresh_devices_statusinfo(0) // stop refreshing while reloading list
  var dfd = $.Deferred();
  $.mobile.loading( "show", {
    text: "reloading device list...",
    textVisible: true,
    theme: "b"
  });
  refresh_zones_and_groups().done(function() {
    // query list
    alertError(apiCall({
      "method":"getProperty",
      "dSUID":"root",
      "query":{
        "x-p44-vdcs":{ "":{ "model":null, "dSUID":null, "deviceIconName":null, "name":null, "displayId":null, "implementationId":null, "x-p44-instanceNo":null, "x-p44-statusText":null, "active":null, "x-p44-opStateText":null, "x-p44-opStateLevel":null, "configURL":null,
        "x-p44-devices":{ "":{ "deviceIconName":null, "dSUID":null, "zoneID":null, "name":null, "model":null, "implementationId":null, "displayId":null, "subdevIdx":null, "x-p44-statusText":null, "active":null, "x-p44-opStateText":null, "x-p44-opStateLevel":null,
        "buttonInputDescriptions":{"#":null}, "binaryInputDescriptions":{"#":null}, "channelDescriptions":{"#":null}, "sensorDescriptions":{"#":null},
        "deviceActionDescriptions":{"#":null}, "deviceStateDescriptions":{"#":null}, "devicePropertyDescriptions":{"#":null}
        , "x-p44-bridged":null, "x-p44-bridgeable":null, "x-p44-allowBridging":null
        , "modelFeatures":{ "identification":null }
      }}}}}
    }, 60000)).done(function(result) {
      // table header
      var tableHtml =
        '<table data-role="table" id="deviceTable" data-mode="columntoggle"'+
        ' class="ui-body-a ui-shadow table-stripe" ' +
        ' data-column-btn-text="Columns to display..." >' +
        '<thead>' +
         '<tr class="ui-bar-a">' +
           '<th data-priority="1"></th>' +
           '<th></th>' +
           '<th onclick="devicelist_sortby(\'dSUID\')" class="hdrcell dsuidcell' + devicelist_sortedByClass('dSUID') + '" data-priority="4">dSUID</th>' +
           '<th onclick="devicelist_sortby(\'name\')" class="hdrcell namecell' + devicelist_sortedByClass('name') + '">Name</th>' +
           '<th onclick="devicelist_sortby(\'model\')" class="hdrcell modelcell' + devicelist_sortedByClass('model') + '" data-priority="2">Model</th>' +
           '<th onclick="devicelist_sortby(\'displayId\')" class="hdrcell idcell' + devicelist_sortedByClass('displayId') + '" data-priority="3">Hardware-ID</th>' +
          '<th onclick="devicelist_sortby(\'zoneID\')" class="hdrcell zonecell' + devicelist_sortedByClass('zoneID') + '" data-priority="1"><abbr title="Zone/Room">Zone</abbr></td>' +
           '<th onclick="devicelist_sortby(\'x-p44-opStateLevel\')" class="hdrcell statuscell' + devicelist_sortedByClass('x-p44-opStateLevel') + '" data-priority="3">Status</th>' +
           '<th class="hdrcell infocell">Info</th>'
        if (bridgeinfo.started) {
          if (bridgeinfo.bridgetype=='proxy') {
            tableHtml += '<th class "hdrcell bridgecell" data-priority="2"><abbr title="Proxy status">P</abbr></th>'
          }
          else {
            tableHtml += '<th class "hdrcell bridgecell" data-priority="2"><abbr title="Matter bridging status">M</abbr></th>'
          }
        }
        tableHtml +=
           '<th></th>' +
         '</tr>' +
        '</thead>' +
        '<tbody>';
      // iterate rows
      var vdcs = result["x-p44-vdcs"];
      var vdcsArray = Object.keys(vdcs).map(function (key) { return { idx: key, vdc:vdcs[key] }; });
      vdcsArray = vdcsArray.sort(function (a,b) { return a.vdc.model.localeCompare(b.vdc.model); });
      for (var i=0; i<vdcsArray.length; i++) {
        var vdc = vdcsArray[i].vdc;
        var vdcActionButton = '';
        // check for vdc-specific button
        if (vdc['implementationId']=='DALI_Bus_Container') {
          vdcActionButton = '<abbr title="Create group or composite color light..."><a id="daliCreateGroupBtn" onclick="openDialog(\'#daliCreateGroup\');" data-mini="true" data-role="button" data-icon="plus" data-iconpos="left" data-inline="true">Group</a></abbr>';
        }
        else if (vdc['implementationId']=='Static_Device_Container') {
          vdcActionButton = '<abbr title="Create new device..."><a onClick="openDialog(\'#customioCreateDevice\');" data-mini="true" data-role="button" data-icon="plus" data-iconpos="left" data-inline="true">Device</a></abbr>';
        }
        else if (vdc['implementationId']=='Evaluator_Device_Container') {
          vdcActionButton = '<abbr title="Create new evaluator..."><a onClick="openDialog(\'#evaluatorCreateDevice\');" data-mini="true" data-role="button" data-icon="plus" data-iconpos="left" data-inline="true">Evaluator</a></abbr>';
        }
        else if (vdc['implementationId']=='Scripted_Device_Container') {
          vdcActionButton = '<abbr title="Create new custom scripteddevice..."><a id="scriptedDeviceAddBtn" onClick="openDialog(\'#scriptedDeviceCreate\');" data-mini="true" data-role="button" data-icon="plus" data-iconpos="left" data-inline="true">Scripted</a></abbr>';
        }
        else if (vdc['implementationId']=='Bridge_Device_Container') {
          if ((!bridgeinfo || !bridgeinfo.connected) && Object.keys(vdc['x-p44-devices']).length==0) continue; // hide empty bridge VDC as long as bridge is not yet up
          vdcActionButton = '<abbr title="Create new bridging device..."><a onClick="openDialog(\'#bridgeCreateDevice\');" data-mini="true" data-role="button" data-icon="plus" data-iconpos="left" data-inline="true">Bridge</a></abbr>';
        }
        else if (vdc['implementationId']=='Proxy_Device_Container') {
          vdcActionButton = '<abbr title="Open WebUI of proxied P44 device"><a target="_blank" href="' + vdc.configURL + '" data-mini="true" data-role="button" data-icon="arrow-r" data-iconpos="left" data-inline="true">WebUI</a></abbr>';
        }
        else if (vdc['implementationId']=='OLA_Device_Container') {
          vdcActionButton = '<abbr title="Create DMX device..."><a onClick="openDialog(\'#dmxCreateDevice\');" data-mini="true" data-role="button" data-icon="plus" data-iconpos="left" data-inline="true">Device</a></abbr>';
        }
        else if (vdc['implementationId']=='LedChain_Device_Container') {
          vdcActionButton = '<abbr title="Create LED chain device..."><a onclick="openDialog(\'#rgbchainCreateDevice\');" data-mini="true" data-role="button" data-icon="plus" data-iconpos="left" data-inline="true">Device</a></abbr>';
        }
        else if (vdc['implementationId']=='EnOcean_Bus_Container') {
          // Later, once this is a release feature we'll have a button here. For now, we need a shift-click in vdc info for that
          //vdcActionButton = '<a id="enoceanDeviceAddBtn" onclick="openDialog(\'#enoceanCreateDevice\');" data-rel="popup" data-position-to="window" data-transition="pop" data-mini="true" data-role="button" data-icon="plus" data-iconpos="left" data-inline="true">Device</a>';
        }
        else if (vdc['implementationId']=='DALI_Bus_Container') {
          // Later, once this is a release feature we'll have a button here. For now, we need a shift-click in vdc info for that
          //vdcActionButton = '<a id="daliinputDeviceAddBtn" onclick="openCreateDaliInput()";" data-rel="popup" data-position-to="window" data-transition="pop" data-mini="true" data-role="button" data-icon="plus" data-iconpos="left" data-inline="true">Device</a>';
        }
        // prepare list of devices in vdc first, so we can display the count
        var devices = vdc["x-p44-devices"];
        var devicesArray = Object.keys(devices).map(function (key) { return { idx: key, device:devices[key] }; });
        devicesArray = devicesArray.sort(deviceCompare);
        // now create the vdc header line
        tableHtml +=
          '<tr class="vdcrow">' +
          '<td class="vdccell iconcell"><img src="/icons/icon16/' + vdc.deviceIconName + '.png" /></td>' +
          '<td class="vdccell iconcell">' + devicesArray.length.toString() + '</td>' +
          '<td class="vdccell dsuidcell' + (vdc.active ? '' : ' notpresent') + '"><abbr title="' + vdc.dSUID + '">' + dsuidShort(vdc.dSUID) + '</abbr></td>' +
          '<td class="vdccell namecell' + (vdc.active ? '' : ' notpresent') + '">' + vdc.name + '</td>' +
          '<td class="vdccell modelcell' + (vdc.active ? '' : ' notpresent') + '">' + vdc.model + '</td>' +
          '<td class="vdccell idcell' + (vdc.active ? '' : ' notpresent') + '">' + dispStr(vdc.displayId)  + '</td>' +
          '<td class="vdccell zonecell' + (vdc.active ? '' : ' notpresent') + '">&nbsp;</td>' +
          '<td class="vdccell statuscell' + (vdc.active ? '' : ' notpresent') + '">' + opStateSpan(vdc['x-p44-opStateLevel'], vdc['x-p44-opStateText'], vdc.active) + '</td>' +
          '<td colspan="' + (bridgeinfo.started ? 3 : 2) + '" class="vdccell actioncell">' +
            vdcActionButton +
            '<abbr title="Open vDC info & settings..."><a dSUID="' + vdc.dSUID + '" class="p44-longclickable" p44-handler="openVdcInfo" data-mini="true" data-role="button" data-icon="info" data-iconpos="notext" data-inline="true">Info</a></abbr></td>' +
          '</td>' +
          '</tr>';
        for (var j=0; j<devicesArray.length; j++) {
          var device = devicesArray[j].device;
          if (device.dSUID==focusDSUID) {
            tableHtml += '<tr class="focusdevice">';
          }
          else {
            tableHtml += '<tr>';
          }
          tableHtml +=
            '<td class="devcell iconcell">&nbsp;</td>' +
            '<td class="devcell iconcell"><img src="/icons/icon16/' + device.deviceIconName + '.png" /></td>' +
            '<td class="devcell dsuidcell' + (device.active ? '' : ' notpresent') + '"><abbr title="' + device.dSUID + '">' + dsuidShort(device.dSUID) + '</abbr></td>' +
            '<td class="devcell namecell' + (device.active ? '' : ' notpresent') + '">' +
              '<abbr title="Rename device..."><a class="renamebutton" onclick="openRenameAddressable(\'' + device.dSUID + '\');" data-mini="true" data-role="button" data-icon="edit" data-iconpos="notext" data-theme="a" data-inline="true">Rename</a></abbr> ' +
              device.name +
            '</td>' +
            '<td class="devcell modelcell' + (device.active ? '' : ' notpresent') + '">' + device.model + '</td>' +
            '<td class="devcell idcell' + (device.active ? '' : ' notpresent') + '">' + dispStr(device.displayId+(device.subdevIdx>0 ? '-'+device.subdevIdx.toString() : ''))  + '</td>' +
            '<td class="devcell zonecell' + (device.active ? '' : ' notpresent') + '">' + dispStr(zonename(device.zoneID)) + '</td>' +
            '<td class="devcell statuscell" id="' + device.dSUID + '_status">' + opStateSpan(device['x-p44-opStateLevel'], device['x-p44-opStateText'], device.active) + '</td>' +
            '<td class="devcell infocell' + (device.active ? '' : ' notpresent') + '" id="' + device.dSUID + '_info">' + device['x-p44-statusText'] + '</td>'
            if (bridgeinfo.started) {
              var br = bridgeinfo.bridgetype=='proxy' ? '🔶' : '🟢'
              tableHtml += '<td class="devcell bridgecell">' + (device['x-p44-bridged'] ? br : (device['x-p44-bridgeable'] ? '🟡' : '&nbsp;')) + '</td>'
            }
          tableHtml +=
            '<td class="devcell actioncell">';
          if (device.modelFeatures.identification) {
            tableHtml += '<abbr title="Identify device (e.g. by blinking)"><a onclick="event.stopPropagation(); getAttention(\'' + device.dSUID + '\');" data-mini="true" data-role="button" data-icon="eye" data-iconpos="notext" data-inline="true">Identify</a></abbr>';
          }
          if (device.channelDescriptions['#']>0) {
            tableHtml += '<abbr title="Show output channels..."><a dSUID="' + device.dSUID + '" class="p44-longclickable" p44-handler="openDeviceChannels" data-mini="true" data-role="button" data-icon="gear" data-iconpos="notext" data-inline="true">Outputs</a></abbr>';
          }
          if (device.deviceActionDescriptions!=undefined) {
            tableHtml += '<abbr title="Device actions..."><a dSUID="' + device.dSUID + '" onclick="openSingleDevice(event);" data-mini="true" data-role="button" data-icon="star" data-iconpos="notext" data-inline="true">Single Device</a></abbr>';
          }
          if (device.sensorDescriptions['#']>0 || device.binaryInputDescriptions['#']>0 || (device.deviceStateDescriptions && device.deviceStateDescriptions['#']>0) || (device.devicePropertyDescriptions && device.devicePropertyDescriptions['#']>0)) {
            tableHtml += '<abbr title="Show sensors and inputs..."><a dSUID="' + device.dSUID + '" onclick="openDeviceInputs(event);" data-mini="true" data-role="button" data-icon="bars" data-iconpos="notext" data-inline="true">Inputs</a></abbr>';
          }
          if (device.buttonInputDescriptions['#']>0) {
            tableHtml += '<abbr title="Edit Button Settings..."><a dSUID="' + device.dSUID + '" onclick="openButtonSettings(event);" data-mini="true" data-role="button" data-icon="grid" data-iconpos="notext" data-inline="true">Button</a></abbr>';
          }
          if (device['implementationId']=='evaluator') {
            tableHtml += '<abbr title="Edit Calculations..."><a dSUID="' + device.dSUID + '" onclick="openEvaluatorEdit(event);" data-mini="true" data-role="button" data-icon="edit" data-iconpos="notext" data-inline="true">Evaluator</a></abbr>';
          }
          if (device['implementationId']=='scripted') {
            tableHtml += '<abbr title="Edit implementation script..."><a dSUID="' + device.dSUID + '" onclick="openScriptedDeviceEdit(event)" data-mini="true" data-role="button" data-icon="edit" data-iconpos="notext" data-inline="true">Script</a></abbr>';
          }
          tableHtml +=
            '<abbr title="Open device info & settings..."><a dSUID="' + device.dSUID + '" class="p44-longclickable" p44-handler="openDeviceInfo" data-mini="true" data-role="button" data-icon="info" data-iconpos="notext" data-inline="true">Info</a></abbr>' +
            '</td></tr>';
        }
      }
      // finalisation of table
      tableHtml +=
        '</tbody>' +
        '</table>';
      $("#deviceList").html(tableHtml).trigger('create');
      install_longclickables("#deviceList");
      $.mobile.loading( "hide" );
      schedule_refresh_devices_statusinfo(15000); // (re)start refreshing status info
      dfd.resolve();
    });
  });
  return dfd.promise();
}




// System Page
// ===========


// IP Validation
function check_ip_edit(id, nonnull)
{
  var value = $(id).val();
  var split = value.split('.');
  var validated = true;
  var allnull = nonnull===true; // set to not-all-null from beginning if we don't want to check for allnull
  if (split.length != 4)
    validated = false;
  else {
    for (var i=0; i<split.length; i++) {
      var s = split[i];
      if (s.length==0 || isNaN(s) || s<0 || s>255) {
        validated = false;
        break;
      }
      if (Number(s)!=0) allnull = false;
    }
    if (allnull) validated = false;
  }
  if (!validated) {
    alert(value + " is not a valid IP address");
    $(id).focus();
  }
  return validated;
};



function restartAfterIPChange() {
  setTimeout(function() {
    closeDialog(function() {
      system_restart();
    });
  }, 200 );
}


function waitForRestart(dialogId)
{
  openDialog(dialogId);
  var waitTime = parseInt(devinfo.PRODUCT_RESTART_TIME)*1000;
  if (!(waitTime>1000)) waitTime = 55000;
  startTimeProgress($(dialogId+'Progress'), waitTime);
  // - reload page after a while
  setTimeout(function() {
    if (restartLocation!=window.location.href) {
      // different URL, change to that without registering in history
      window.location.replace(restartLocation); // triggers reload
    }
    else {
      window.location.reload(true); // some docs say a boolean true actually force-reloads (but many don't)
    }
  }, waitTime );
}


function system_restart()
{
  closeDialog();
  p44mCall({ cmd:'restart' }).done(function() {
    waitForRestart("#restartWait");
  });
}

function system_shutdown()
{
  p44mCall({ cmd:'poweroff' }).done(function() {
    openDialog("#shutdownInfo");
  });
}


function system_factoryreset(mode)
{
  closeDialog();
  // trigger factory reset
  p44mCall({ cmd:'factoryreset', mode:mode.toString() }).done(function() {
    // NOP for now
  });
  setTimeout(function() {
    waitForRestart("#factoryResetWait");
  }, 100 );
}



function system_backup_config()
{
  location.href = p44mGetUri("cmd=configbackup");
  setTimeout(function() {
    alert("Configuration has been downloaded - please store file in a safe place");
  }, 1500 );
}


function system_restore_config()
{
  closeDialog();
  var file = $('#configArchiveFile').get(0).files[0];
  var data = new FormData();
  data.append("upload_file", file);
  $.ajax({
    url: constructUri('/cfg/upload/', 'cmd=configrestoreprep'),
    data: data,
    contentType: false, // important!
    processData: false,
    timeout: 10000,
    method: 'POST'
  }).done(function(response) {
    if (response.error!=undefined) {
      alert('Restore failed: ' + response.error.message);
    }
    else {
      // preparation ok
      var result = response.result;
      // - update info
      if (result.oldarchive) {
        // archive without info
        $('#restoreDeviceInfo').html('<ul><li>Original device unknown</li></ul>');
        $('#restoreCompatWarnings').html('<li>Old backup file (prior to firmware 1.6.0.6)</li>');
      }
      else {
        // archive with info
        var infoHtml =
          'The backup you are about to restore was made<ul>' +
          '<li>at <b>' + result.time + '</b>';
        if (result.differentserial || result.differentmodel) {
          infoHtml += '<li>on a <b>' + result.model + '</b> (GTIN:' + result.gtin + ')';
          infoHtml += '<li>with serial number <b>' + result.serial + '</b>';
        }
        else {
          infoHtml += '<li>on <b>this ' + result.model + '</b>';
        }
        infoHtml += '<li>with firmware version <b>' + result.version + '</b> installed at the time of backup.</ul>';
        $('#restoreDeviceInfo').html(infoHtml);
        var warningHtml = '';
        if (result.differentmodel)
          warningHtml += '<li>The backup <b>is from a different product type</b> - it will probably not work ok</li>';
        else if (result.differentserial)
          warningHtml += '<li>The backup <b>was made on another device than this one</b> - restore only if this unit is a <b>replacement</b> unit for the original, which means, <span class="errortext"><b>the original unit MUST NOT be connected to the system any more!</b></span></li>';
        if (result.oldfirmware)
          warningHtml += '<li>This device <b>has older firmware than the device the backup was made with</b> - it will probably not work ok</li>';
        $('#restoreCompatWarnings').html(warningHtml);
      }
      // - show confirm dialog
      openDialog('#configRestoreConfirm');
    }
  }).fail(function(jqXHR, textStatus, errorThrown) {
    alert('Configuration upload failed: '+textStatus);
  });
}


function system_restore_apply(mode)
{
  closeDialog();
  // trigger config restore apply
  p44mCall({ cmd:'configrestoreapply', mode:mode.toString() }).done(function() {
    // NOP for now
  });
  // if not cancel, wait for restart
  if (mode!=0) {
    setTimeout(function() {
      waitForRestart("#restoreConfigWait");
    }, 100 );
  }
}





function update_device_titles()
{
  var m = devinfo.PRODUCT_MODEL;
  // model with dS given name
  if (bridgename.length>0) m += ' "' + bridgename + '"';
  $('#html_title_model').html(m);
  $('#system_model').html(m);
  // model name in about (title screen)
  $('#about_title_model').html(devinfo.PRODUCT_MODEL);
  // other info
  $('#about_copyright_years').html(devinfo.PRODUCT_COPYRIGHT_YEARS);
  $('#about_copyright_link').html(devinfo.PRODUCT_COPYRIGHT_HOLDER);
  $('#about_copyright_link').attr("href", devinfo.PRODUCT_INFORMATION_PAGE_LINK);
  $('#about_model_description').html(devinfo.PRODUCT_MODEL_DESCRIPTION);
  $('#devices_compatpage_link').attr("href", devinfo.PRODUCT_COMPATIBILITY_PAGE_LINK);
  // vdchost's dSUID
  $('#vdchostdsuid').html(vdchostdsuid);
}



function refresh_sysinfo()
{
  var dfd = $.Deferred();
  // platform info
  alertError(p44mCall({ cmd:'devinfo' })).done(function(result) {
    devinfo = result;
    // update global vars
    // - make sure we display the device's localtime (so we need to compensate for our own local offset)
    var d = new Date;
    tickDiff = devinfo.localtimetick*1000-d.getTime()+d.getTimezoneOffset()*60*1000;
    // update HTML
    var isDIY = devinfo.PRODUCT_IS_DIY=="1";
    var hasOLA = devinfo.PRODUCT_HAS_OLA=="1";
    var hasLED = devinfo.PRODUCT_HAS_LEDCHAIN=="1";
    var hasKite = devinfo.PRODUCT_KITE_FRONTEND!=undefined;
    var isOpenWrt = devinfo.PLATFORM_OS_IDENTIFIER=="openwrt";
    var versionText = devinfo.FIRMWARE_VERSION;
    if (devinfo.FIRMWARE_FEED!='prod') versionText += ' (' + devinfo.FIRMWARE_FEED + ')';
    var nextVers = '';
    if (devinfo.STATUS_NEXT_FIRMWARE && devinfo.STATUS_NEXT_FIRMWARE.length>0) nextVers = ', <span class="infoalert">available: <span class="infovalue">' + devinfo.STATUS_NEXT_FIRMWARE + '</span></span>';
    var uphours = devinfo.uptime/3600;
    var formattedUptime =
      String(Math.floor(uphours/24)) + ' days ' + String(Math.floor(uphours%24)) + ' hours';
    var sysinfo =
      '<p>Product: <abbr title="' + devinfo.PLATFORM_NAME.replace(/"/g,"&quot;") + '"><span class="infovalue" id="system_model">' + devinfo.PRODUCT_MODEL + '</span></abbr>' +
      ' - <a target="_blank" href="' + devinfo.PRODUCT_INFORMATION_PAGE_LINK +'">product page</a></p>' +
      '<p>Firmware Version: <span class="infovalue">' + versionText + '</span>' +
      ' - <a target="_blank" href="' + devinfo.PRODUCT_RELEASENOTES_PAGE_LINK +'">release notes</a>' + nextVers + '</p>' +
      '<p>Serial Number: <span class="infovalue"><abbr title="' + devinfo.PRODUCER.replace(/"/g,"&quot;") + '">' + devinfo.UNIT_SERIALNO + '</abbr></span></p>'
    if (devinfo.PRODUCT_GTIN && devinfo.PRODUCT_GTIN!=="none" && devinfo.PRODUCT_GTIN.length>0) {
      sysinfo += '<p>GTIN: <span class="infovalue">' + devinfo.PRODUCT_GTIN + '</span></p>'
    }
    sysinfo +=
      '<p>MAC: <span class="infovalue">' + devinfo.UNIT_MACADDRESS + '</span></p>' +
      '<p>System uptime: <span class="infovalue">' + formattedUptime + '</span></p>';
    $('#systemInfo').html(sysinfo);
    refresh_clock();
    // update model names
    update_device_titles();
    // enable some extra elements depending on product
    showIf(isOpenWrt, '#opensource_openwrt');
    showIf(hasOLA,'#opensource_ola');
    showIf(hasLED,'#openLEDSimButton');
    showIf(isDIY,'#diyWarning');
    // query name and dSUID of vdc host
    apiCall({
      "method":"getProperty",
      "dSUID":"root",
      "query":{ "dSUID":null, "name":null, "x-p44-bridge":null, "x-p44-scenesList":null }
    }).done(function(result) {
      bridgename = result.name
      // Always use model from devinfo, not from vdcd
      vdchostdsuid = result.dSUID
      // also save bridge status
      bridgeinfo = result['x-p44-bridge']
      if (bridgeinfo) {
        showIf(bridgeinfo.bridgetype=='matter', '#p44mbrdlogs');
      }
      // and sorted scenes list
      var scenes = result['x-p44-scenesList']
      var sa = Object.keys(scenes).map(function (key) {
        return {
          index: scenes[key].index,
          sceneNo: key,
          name:scenes[key].name,
          kind:scenes[key].kind
        };
      });
      sceneslist = sa.sort(function (a,b) { return a.index-b.index; });
      // and titles
      update_device_titles()
    }).always(function() {
      dfd.resolve();
    });
  });
  return dfd.promise();
}

function refresh_clock()
{
  if (typeof(tickDiff)=="number") {
    var date = new Date((new Date).getTime()+tickDiff);
    var formattedTime =
      date.getFullYear() + '-' +
      ('00'+String(date.getMonth()+1)).substr(-2) + '-' +
      ('00'+String(date.getDate())).substr(-2) + ' ' +
      ('00'+String(date.getHours())).substr(-2) + ':' +
      ('00'+String(date.getMinutes())).substr(-2) + ':' +
      ('00'+String(date.getSeconds())).substr(-2);
    $('#clock').html(
      '<p>System Date/Time: <span class="infovalue">' + formattedTime + '</span></p>'
    );
  }
  // schedule next refresh
  setTimeout(function() { refresh_clock(); }, 500 );
}


function refresh_ipconfig()
{
  var dfd = $.Deferred();
  alertError(p44mCall({ cmd:'ipconfig' })).done(function(result) {
    var ipdisphtml = '';
    // DHCP or static
    if (result.dhcp) {
      ipdisphtml =
        '<p>DHCP: <span class="infovalue">on</span></p>' +
        '<p>Current IP address: <span class="infovalue">' + result.currentip + '</span></p>';
      $('#dhcp').val(1);
    }
    else {
      ipdisphtml =
        '<p>DHCP: <span class="infovalue">off</span></p>' +
        '<p>Configured IP address: <span class="infovalue">' + result.ipaddr + '</span></p>' +
        '<p>Net Mask: <span class="infovalue">' + result.netmask + '</span></p>' +
        '<p>Default Gateway IP: <span class="infovalue">' + result.gatewayip + '</span></p>' +
        '<p>DNS server 1: <span class="infovalue">' + result.dnsip + '</span></p>' +
        '<p>DNS server 2: <span class="infovalue">' + result.dnsip2 + '</span></p>';
      $('#dhcp').val(0);
    }
    $('#dhcp').slider("refresh");
    if (result.ipv6) {
      $('#ipv6').val(1);
      ipdisphtml += '<p>IPv6 link local address: <span class="infovalue">' + result.ipv6_link + '</span></p>'
      if (result.ipv6_global) {
        ipdisphtml += '<p>IPv6 global address: <span class="infovalue">' + result.ipv6_global + '</span></p>'
      }
    }
    else {
      $('#ipv6').val(0);
    }
    $('#ipv6').slider("refresh");
    $('#ipconfig').html(ipdisphtml);

    // set IP fields
    $("#ipAddr").val(result.ipaddr);
    $("#ipMask").val(result.netmask);
    $("#ipGW").val(result.gatewayip);
    $("#ipDNS1").val(result.dnsip);
    $("#ipDNS2").val(result.dnsip2);
    // refresh dhcp switch dependencies
    refresh_dhcp_deps();
    // enable edit button now
    $('#ipeditbtn').removeClass('ui-disabled');
  }).always(function() {
    dfd.resolve();
  });
  return dfd.promise();
}



function refresh_dhcp_deps()
{
  // refresh DHCP switch dependencies
  if ($("#dhcp").val()>0) {
    $("#ipAddr").textinput("disable");
    $("#ipMask").textinput("disable");
    $("#ipGW").textinput("disable");
    $("#ipDNS1").textinput("disable");
    $("#ipDNS2").textinput("disable");
  }
  else {
    $("#ipAddr").textinput("enable");
    $("#ipMask").textinput("enable");
    $("#ipGW").textinput("enable");
    $("#ipDNS1").textinput("enable");
    $("#ipDNS2").textinput("enable");
  }
}


var netEditWaitTimer = null;

function openNetworkConfig(event, hiddenFeatures)
{
  if (hiddenFeatures==1 || (hiddenFeatures && event.shiftKey)) {
    // hidden wifi settings
    refresh_wificonfig().done(function() {
      openDialog('#wifiedit');
    });
    return;
  }
  openDialog('#ipedit');
}



function refresh_wificonfig()
{
  var dfd = $.Deferred();
  alertError(p44mCall({ cmd:'wificonfig' })).done(function(result) {
    // set WiFi fields
    // - client
    $("#wifi_cli").val(result.cli.enabled ? 1 : 0).slider("refresh");
    $("#cli_ssid").val(result.cli.ssid);
    $("#cli_sec").val(result.cli.encryption).selectmenu("refresh");
    $("#cli_key").val(result.cli.key);
    // - AP
    $("#wifi_ap").val(result.ap.enabled ? 1 : 0).slider("refresh");
    $("#ap_ssid").val(result.ap.ssid);
    $("#ap_sec").val(result.ap.encryption).selectmenu("refresh");
    $("#ap_key").val(result.ap.key);
    refresh_wifimode_deps()
  }).always(function() {
    dfd.resolve();
  });
  return dfd.promise();
}


function refresh_wifimode_deps()
{
  showIf($("#wifi_cli").val()=="1", "#cli_params");
  showIf($("#wifi_ap").val()=="1", "#ap_params");
}


function applyWifiSettings()
{
  p44mCall({
    "cmd":'wificonfig',
    "cli" : {
      "enabled": $("#wifi_cli").val()=="1" ? true : false,
      "ssid": $("#cli_ssid").val(),
      "encryption": $("#cli_sec").val(),
      "key": $("#cli_key").val()
    },
    "ap" : {
      "enabled": $("#wifi_ap").val()=="1" ? true : false,
      "ssid": $("#ap_ssid").val(),
      "encryption": $("#ap_sec").val(),
      "key": $("#ap_key").val()
    }
  }, 30000).done(function() {
    // has not broken connectivity, close whatever dialog is open now (still edit or netEditWait)
    clearTimeout(netEditWaitTimer);
    closeDialog();
  });
  // close edit dialog and show neteditWait instead
  netEditWaitTimer = setTimeout(function() {
    closeDialog(function () {
      openDialog("#neteditWait");
      netEditWaitTimer = setTimeout(function() {
        // after 30 seconds, try a reload
        window.location.reload(true); // some docs say a boolean true actually force-reloads (but many don't)
      }, 30000)
    });
  }, 100 );
}


function applyIpSettings()
{
  var dhcp = $("#dhcp").val()!=0
  var ipv6 = $("#ipv6").val()!=0
  if (
    check_ip_edit("#ipAddr", !dhcp) &&
    check_ip_edit("#ipMask") &&
    check_ip_edit("#ipGW") &&
    check_ip_edit("#ipDNS1") &&
    check_ip_edit("#ipDNS2")
  ) {
    alertError(p44mCall({
      "cmd":'ipconfig',
      "dhcp":dhcp ? 1 : 0,
      "ipv6":ipv6 ? 1 : 0,
      "ipaddr": $("#ipAddr").val(),
      "netmask": $("#ipMask").val(),
      "gatewayip": $("#ipGW").val(),
      "dnsip": $("#ipDNS1").val(),
      "dnsip2": $("#ipDNS2").val()
    }, 30000).always(function() {
      // make sure the neteditWait does not appear (in case of very quick response)
      clearTimeout(netEditWaitTimer);
      // close the wait popup in case it is open
      closeDialog();
    }).done(function() {
      // update restart location
      if ($("#dhcp").val()==0) {
        // configured IP will become active, use that (note that port must be 80)
        restartLocation = 'http://' + $("#ipAddr").val();
      }
      // update IP config
      refresh_ipconfig();
      // ask for restart
      setTimeout(function() { openDialog("#ipRestartAsk"); }, 100 );
    }));
    setTimeout(function() {
      closeDialog();
      $("#ipedit" ).off('popupafterclose.ipedit');
      $("#ipedit" ).on('popupafterclose.ipedit', function() {
        $("#ipedit" ).off('popupafterclose.ipedit');
        netEditWaitTimer = setTimeout(function() { openDialog("#neteditWait"); }, 100 );
      });
    }, 100 );
  }
}


function setNewPassword()
{
  // get data
  var newPassword = $("#newPassword").val().toString();
  var verifyPassword = $("#verifyPassword").val().toString();
  // clear fields
  $("#newPassword").val("");
  $("#verifyPassword").val("");
  if (
    (newPassword != verifyPassword) ||
    (newPassword.length<3)
  ) {
    alert("Password ist shorter than 3 chars or verification input does not match password");
  }
  else {
    alertError(p44mCall({
      "cmd":'setpassword',
      "password": newPassword
    }, 30000)).always(function() {
      closeDialog();
    });
  }
}



function openNameEdit(event, hiddenFeatures) {
  if (hiddenFeatures) {
    // hidden remote access
    openRemoteAccess();
    return;
  }
  $("#newBridgeName").val(bridgename);
  openDialog('#nameedit');
}

function setBridgeName()
{
  // get data
  bridgename = $("#newBridgeName").val().toString();
  // set name of vdc host
  apiCall({
    "method":"setProperty",
    "dSUID":"root",
    "properties":{ "name":bridgename }
  }).always(function() {
    closeDialog();
    // update titles
    update_device_titles();
  });
}



function openTzLocationEdit(event, hiddenFeatures)
{
  if (hiddenFeatures==2) {
    // key modifiers
    if (event.shiftKey || hiddenFeatures==1) {
      openREPL()
      return
    }
    openMainScript()
    return
  }
  else if (hiddenFeatures==1) {
    // long click
    openDevTabs()
    return
  }
  alertError(apiCall({
    "method":"getProperty",
    "dSUID":"root",
    "query":{ "x-p44-latitude":null, "x-p44-longitude":null }
  })).done(function(result) {
    $('#longitudeEdit').val(result['x-p44-longitude']);
    $('#latitudeEdit').val(result['x-p44-latitude']);
  }).always(function() {
    alertError(p44mCall({ cmd:'tzconfig' })).done(function(result) {
      $('#timeZoneSelect').val(result.timezonename).selectmenu("refresh");
      openDialog('#tzlocationedit');
    });
  });
}

function setTzLocation()
{
  alertError(apiCall({
    "method":"setProperty",
    "dSUID":"root",
    "properties":{ "x-p44-latitude":$('#latitudeEdit').val(), "x-p44-longitude":$('#longitudeEdit').val() }
  })).always(function() {
    var tzn = $('#timeZoneSelect').val();
    alertError(p44mCall({ cmd:'tzconfig', timezonename:tzn })).always(function() {
      closeDialog(function() {
        // reload modified device time offsets
        refresh_sysinfo();
      });
    });
  });
}




// Log page
// ========

function refresh_vdcd_log()
{
  $.get("vdcd_tail_log.txt", function(data) {
    $("#vdcd_tail_log").html(data);
  });
}

function toggleLogWrap()
{
  $("#vdcd_tail_log").toggleClass('logwrap')
}


function set_vdcd_loglevel(i)
{
  apiCall({
    "method":"loglevel",
    "dSUID":"root",
    "value":i
  }).done(function(response) {
    // refresh log
    refresh_vdcd_log();
  });
}



