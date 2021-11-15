// functions to abstract the actual API


// ubus state
var ubus_session = false;
var pendingCalls = false;
var rpcid = 1;

function apiCall(jsonquery, timeout, loginWhenNeeded, endpoint)
{
  if (loginWhenNeeded==undefined) loginWhenNeeded=true;
  if (endpoint!=undefined) {
    // convert to subsystem call
    jsonquery = { [endpoint]: jsonquery };
  }
  var dfd = $.Deferred();
  var uc = ubusCall("kksdcmd", "api", jsonquery, timeout, loginWhenNeeded).done(function(response) {
    if (response.result!==undefined) {
      dfd.resolve(response.result);
    }
    else if (response.error!==undefined) {
      dfd.reject(response.errordomain, response.error, response.errormessage, response.userfacingmessage);
    }
    else {
      dfd.reject("local", -1, "api call got no json response");
    }
  }).fail(function(domain, code, message) {
    dfd.reject(domain, code, message);
    console.log('apiCall: [' + domain + '] Error ' + code.toString() + ': ' + message);
  });
  var promise = dfd.promise();
  promise.abort = function() { uc.abort(); }
  return promise;
}


function logOut()
{
  ubus_session = false;
  askForLogin();
}


function addSession(locationhash)
{
  if (ubus_session) locationhash = ubus_session + '$' + locationhash;
  return locationhash;
}


function extractSession(locationhash)
{
  var i = locationhash.indexOf('$');
  if (i>0) {
    ubus_session = locationhash.substring(0,i);
  }
  if (i>=0) {
    locationhash = locationhash.substring(i+1);
  }
  // return remaining locationhash
  return locationhash;
}


function askForLogin(loginMsg)
{
  if (loginMsg==undefined) loginMsg='';
  var dfd = $.Deferred();
  if (pendingCalls===false) pendingCalls = []; // existence of array means login in progress
  openDialog('#loginDialog', function() {
    $('#loginMessage').html('<span class="errortext">' + escapehtml(loginMsg) + '</span>');
    $('#loginPassword').val(''); // always reset password
    $('#loginCheckButton').off('click.login');
    $('#loginCheckButton').on('click.login', function() {
      // attempt ubus login
      var usr = $('#loginUser').val();
      var pwd = $('#loginPassword').val();
      ubusRawCall(
        "00000000000000000000000000000000", "session", "login",
        { "username": usr, "password": pwd }
      ).always(function() {
        closeDialog();
      }).done(function(response) {
        // successful login
        ubus_session = response.ubus_rpc_session;
        dfd.resolve();
      }).fail(function(domain, code, message) {
        // some kind of login failure
        if (domain=="ubus" && code==6) {
          // 6 == UBUS_STATUS_PERMISSION_DENIED -> actually wrong password - repeat asking
          askForLogin("Invalid login user and/or password").done(function(response) {
            dfd.resolve(response);
          }).fail(function(domain, code, message) {
            dfd.reject(domain, code, message);
          });
          return;
        }
        dfd.reject(domain, code, message);
      });
    });
  });
  return dfd.promise();
}


function ubusCall(path, method, params, timeout, loginWhenNeeded)
{
  if (loginWhenNeeded==undefined) loginWhenNeeded=true;
  var dfd = $.Deferred();

  // queue calls until we have a ubus session
  if (ubus_session===false) {
    if (loginWhenNeeded) {
      var loggingIn = pendingCalls!==false; // pending calls means login is already in progress
      if (!loggingIn) pendingCalls = [];
      if (path.length>0) {
        pendingCalls.push({ "path":path, "method": method, "params":params, "timeout":timeout, "dfd":dfd });
      }
      if (!loggingIn) {
        // login not yet in progress - initiate it, but with a slight delay to stabilize UI first
        setTimeout(function() {
          askForLogin().done(function() {
            // process pending calls
            if (path.length<=0) dfd.resolve(); // pseudo-call just for logging in
            while(pendingCalls.length>0) {
              var c = pendingCalls.shift();
              ubusRawCall(ubus_session, c.path, c.method, c.params, c.timeout).done(function(response) {
                c.dfd.resolve(response);
              }).fail(function(domain, code, message) {
                c.dfd.reject(domain, code, message);
              });
            }
            pendingCalls = false;
          }).fail(function(domain, code, message) {
            pendingCalls = false; // forget all pending calls
            dfd.reject(domain, code, message);
          });
        }, 100 );
      }
    }
    else {
      dfd.reject("local", -1, "no session");
    }
  }
  else {
    // session exists
    var ruc = ubusRawCall(ubus_session, path, method, params, timeout).done(function(response) {
      dfd.resolve(response);
    }).fail(function(domain, code, message) {
      // check for auth error
      if (domain=="jsonrpc" && code==-32002) {
        // uhttpd access denied (session expired, for example)
        ubus_session = false; // forget invalid session
        if (loginWhenNeeded) {
          // repeat with re-login
          ubusCall(path, method, params, timeout, true).done(function(response) {
            dfd.resolve(response);
          }).fail(function(domain, code, message) {
            dfd.reject(domain, code, message);
          });
          return;
        }
      }
      dfd.reject(domain, code, message);
    });
    var promise = dfd.promise();
    promise.abort = function() { ruc.abort(); }
    return promise;
  }
  return dfd.promise();
}


function ubusRawCall(session, path, method, params, timeout)
{
  var dfd = $.Deferred();
  var usedId = rpcid++;
  // uhttpd ubus API
  // { "jsonrpc": "2.0", "id": 1, "method": "call", "params": [ "sssss", "vdcd", "api", { "param1":1 } ] }
  var ubuscall = {
    "jsonrpc": "2.0",
    "id": usedId,
    "method": "call",
    "params": [
      session,
      path,
      method,
      params
    ]
  };
  var xhr = $.ajax({
    url: '/ubus',
    type: 'post',
    dataType: 'json',
    timeout: timeout,
    data: JSON.stringify(ubuscall)
  }).done(function(response) {
    // technically successful call to ubus, check for auth failures
    if (
      response===undefined ||
      response.jsonrpc!="2.0"
    ) {
      // wrong response format
      dfd.reject("local", -1, "unexpected response format");
    }
    else if (response.id!=usedId) {
      dfd.reject("local", -1, "non-matching response id");
    }
    else if (response.error!==undefined) {
      dfd.reject("jsonrpc", response.error.code, response.error.message);
    }
    else if (
      response.result===undefined ||
      response.result[0]===undefined
    ) {
      dfd.reject("local", -1, "no or unexpected result object");
    }
    else if (response.result[0]!=0) {
      dfd.reject("ubus", response.result[0], "ubus error code");
    }
    else if (response.result[1]===undefined) {
      dfd.reject("local", -1, "no ubus result");
    }
    else {
      // successful call
      dfd.resolve(response.result[1]);
    }
  }).fail(function(jqXHR, textStatus, errorThrown) {
    // unsuccessful at the AJAX API level
    dfd.reject("ajax", jqXHR.status, textStatus + " (" + errorThrown.message + " accessing ubus)");
  });
  var promise = dfd.promise();
  promise.abort = function() { xhr.abort(); }
  return promise;
}
