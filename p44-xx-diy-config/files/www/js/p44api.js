
// Helpers

function showError(domain,code,message)
{
  if (!domain || domain=='ajax') {
    // problem on ajax level = network problem or timeout, usually
    console.log("Ajax/undefined domain error, suggesting reload: ["  + domain + '] Error ' + code.toString() + ': ' + message)
    if (confirm('Web interface is not responding. Try reloading the page?')) {
      window.location.reload(true)
    }
  }
  else {
    alert('[' + domain + '] Error ' + code.toString() + ': ' + message)
  }
}


// standard error handling: show alert
function alertError(theCall)
{
  theCall.fail(showError)
  return theCall
}


// functions to abstract the actual API (ubus via uhttpd, json via mg44)



// mg44 request validation token
var rqvaltok = false;


function addSession(locationhash)
{
  if (rqvaltok && rqvaltok!==true) locationhash = rqvaltok + '$' + locationhash;
  return locationhash;
}


function extractSession(locationhash)
{
  var i = locationhash.indexOf('$');
  if (i>0) {
    rqvaltok = locationhash.substring(0,i);
  }
  if (i>=0) {
    locationhash = locationhash.substring(i+1);
  }
  // return remaining locationhash
  return locationhash;
}


function p44mCall(jsonquery, timeout)
{
  var dfd = $.Deferred();
  mg44Call("/cfg/json/", jsonquery, timeout).done(function(response) {
    if (response.result!==undefined) {
      dfd.resolve(response.result);
    }
    else if (response.error!==undefined) {
      dfd.reject("p44m", response.error.code, response.error.message, "");
    }
    else {
      dfd.reject("local", -1, "api call got no json response");
    }
  }).fail(function(domain, code, message) {
    dfd.reject(domain, code, message);
    console.log('p44mCall: [' + domain + '] Error ' + code.toString() + ': ' + message);
  });
  return dfd.promise();
}


function constructUri(baseuri, uriparams)
{
  sep = "?"
  if (rqvaltok!==true) {
    baseuri += '?rqvaltok=' + rqvaltok;
    sep = "&"
  }
  if (uriparams!==undefined && uriparams.length>0) baseuri += sep + uriparams;
  return baseuri;
}


function p44mGetUri(uriparams)
{
  return constructUri("/cfg/json/", uriparams)
}


function apiCall(jsonquery, timeout, loginWhenNeeded, endpoint)
{
  // Note: loginWhenNeeded not implemented in old style API
  if (endpoint==undefined) endpoint="vdc";
  var dfd = $.Deferred();
  var mg = mg44Call("/api/json/" + endpoint, jsonquery, timeout).done(function(response) {
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
  promise.abort = function() { mg.abort(); }
  return promise;
}


function mg44token()
{
  var dfd = $.Deferred();
  if (rqvaltok!==false) {
    // we have the token
    dfd.resolve()
  }
  else {
    // need to fetch token first
    $.getJSON( '/tok/json' , {
    }).done(function(response) {
      rqvaltok = response;
      // we have the token now
      dfd.resolve()
    }).fail(function(jqXHR, textStatus, errorThrown) {
      if (jqXHR.status=404) {
        // mg44 has no CSRF token enabled
        rqvaltok=true
        dfd.resolve()
      }
      else {
        console.log('TOK error ' + textStatus);
        dfd.reject("mg44", -1, "cannot get token");
      }
    });
  }
  return dfd.promise();
}



function mg44Call(uri, jsonquery, timeout, retrycount)
{
  if (retrycount==undefined) retrycount=0;
  var dfd = $.Deferred();
  var promise = dfd.promise();
  // mg44 based API
  mg44token().done(function () {
    promise.abort = function() { xhr.abort(); }
    var url = constructUri('' + uri);
    var jdata = JSON.stringify(jsonquery);
    var xhr = $.ajax({
      url: url,
      type: 'post',
      dataType: 'json',
      timeout: timeout,
      data: jdata,
      xhrFields: {
        withCredentials: true
      }
    }).done(function(response) {
      dfd.resolve(response)
    }).fail(function(jqXHR, textStatus, errorThrown) {
      // unsuccessful at the AJAX API level
      console.log(`mg44call failure: jqXHR.status=${jqXHR.status} jqXHR.responseText=${jqXHR.responseText} textStatus=${textStatus} errorThrown=${errorThrown}`)
      if (jqXHR.status==200 && textStatus=="parsererror") {
        // It seems that auto-parsing can fail
        console.log("jqXHR JSON autoparsing has failed - do it directly")
        try {
          response = JSON.parse(jqXHR.responseText)
          dfd.resolve(response)
        }
        catch(e) {
          dfd.reject("ajax", 500, "JSON result parsing problem even in second attempt: " + e.message)
        }
      }
      else if (jqXHR.status==0 && textStatus=="error" && retrycount<3) {
        retrycount++
        console.log("status=0/error problem, retrying, count=" + retrycount.toString())
        mg44Call(uri, jsonquery, timeout, retrycount).done(function(response) {
          dfd.resolve(response);
        }).fail(function(domain, code, message) {
          dfd.reject(domain, code, message)
        });
      }
      else if ((jqXHR.status==401 || jqXHR.status==403) && retrycount<3) {
        retrycount++
        // Note: status 403 is usually due to expired token.
        //   But we also force reloading the token at 401, because of a bug in Safari??
        //   where https PUT sometimes lacks Authorization: header repeatedly.
        //   Forcing token reload causes a GET request in between, which seems to
        //   mitigate the issue
        rqvaltok = false // force getting a new token
        console.log("auth problem (status=" + jqXHR.status + "), retrying, count=" + retrycount.toString())
        mg44Call(uri, jsonquery, timeout, retrycount).done(function(response) {
          dfd.resolve(response)
        }).fail(function(domain, code, message) {
          dfd.reject(domain, code, message)
        });
      }
      else {
        dfd.reject("ajax", jqXHR.status, textStatus + " (" + errorThrown.message + " accessing '" + url + "' data='" + jdata + "')")
      }
    });
  }).fail(function(domain, code, message) {
    dfd.reject(domain, code, message)
  })
  return promise;
}
