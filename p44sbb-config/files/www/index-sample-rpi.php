<?php

define("DBGLVL", 0);

// configuration
$jsonapi_host = 'localhost';
$jsonapi_port = 9999;


/// @param $aJsonRequest associative array representing JSON request to send
function jsonApiCall($aUri, $aJsonRequest = false, $aAction = false)
{
  global $jsonapi_host, $jsonapi_port;

  // wrap in mg44-compatible JSON
  $wrappedcall = array(
    'method' => $aAction ? 'POST' : 'GET',
    'uri' => $aUri,
  );
  if ($aJsonRequest!==false) $wrappedcall['data'] = $aJsonRequest;
  $request = json_encode($wrappedcall);
  $fp = fsockopen($jsonapi_host, $jsonapi_port, $errno, $errstr, 10);
  if (!$fp) {
    $result = array('error' => 'cannot open TCP connection to ' . $jsonapi_host . ':' . $jsonapi_port);
  } else {
    fwrite($fp, $request);
    $answer = '';
    while (!feof($fp)) {
      $answer .= fgets($fp, 128);
    }
    fclose($fp);
    // convert JSON to associative php array
    $result = json_decode($answer, true);
  }
  return $result;
}



?>

<!DOCTYPE html>
<html xml:lang="de">

  <head>
    <meta http-equiv="content-type" content="text/html; charset=utf-8">

    <meta name="viewport" content="width=device-width, initial-scale=1">

    <meta name="apple-mobile-web-app-capable" content="yes" />
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent" />

    <title id="title_model">Gleis70 Fallblattanzeiger</title>

    <script src="js/jquery-1.9.1.min.js"></script>

    <style type="text/css">

      body {
        font-family: sans-serif;
        background-color:#00409d;
        color: white;
      }

      #title {
        padding: 0px;
        padding-bottom: 20px;
      }
      #title table {
        width: 100%;
      }
      #projectname { width: 35%; }
      #credits { width: 65%; }
      #credits ul {
        margin-left: 20px;
        list-style: square;
        list-style-position: inside;
        padding: 0;
      }
      #title td {
        vertical-align: top;
        padding: 0;
      }

      #errormessage { font-weight: bold; color: red; }

    </style>



    <script language="javascript1.2" type="text/javascript">

      $(function()
      {
        // document ready
      });

/*
      function updateMachineStatus()
      {
        $.ajax({
          url: '/api.php/machine',
          type: 'get',
          dataType: 'json',
          timeout: 3000
        }).done(function(response) {
          var status = response.result.status;
          // update machine status
          if (status<2) {
            $('#machineerr').show();
            $('#machinerestart').hide();
            $('#machineready').hide();
          }
          else if (status<3) {
            $('#machineerr').hide();
            $('#machinerestart').show();
            $('#machineready').hide();
          }
          else {
            $('#machineerr').hide();
            $('#machinerestart').hide();
            $('#machineready').show();
          }
        });
      }
*/

    </script>

  </head>

  <body>
    <div id="title">
      <table>
        <tr>
          <td id="projectname"><h1>Gleis70 Fallblattanzeiger<br/>@<a href="<?php echo $_SERVER['SCRIPT_NAME']; ?>"><?php echo $_SERVER['SERVER_ADDR']; ?></a></h1></td>
          <td id="credits">
            <p>SBB Fallblattanzeige für Gleis70</p>
            <ul>
              <li>p44sbbd, Web-Interface und setup von <a href="" target="_blank">luz/plan44.ch</a></li>
            </ul>
          </td>
        </tr>
      </table>
    </div>

    <?php

    if (DBGLVL>1) {
      echo('<h3>$_REQUEST</h3><ul>');
      foreach ($_REQUEST as $k => $v) {
        echo('<li>' . $k . ' = ' . $v . '</li>');
      }
      echo('</ul>');

      echo('<h3>$_SERVER</h3><ul>');
      foreach ($_SERVER as $k => $v) {
        echo('<li>' . $k . ' = ' . $v . '</li>');
      }
      echo('</ul>');

/*
      printf ('<b>$_FILES:</b><br><pre>');
      print_r($_FILES);
      printf ("</pre>");
*/
    }

    if (isset($_REQUEST['setsingle'])) {
      // get addr and pos
      $addr = $_REQUEST['addr'];
      $pos = $_REQUEST['pos'];
      jsonApiCall('/module', array(
        'addr' => $addr,
        'pos' => $pos
      ), true);
    }
    else if (isset($_REQUEST['sendhex'])) {
      // directly send hex
      $hex = $_REQUEST['hexbytes'];
      jsonApiCall('/interface', array(
        'sendbytes' => $hex
      ), true);
    }
    else if (isset($_REQUEST['info'])) {
      // all info about module
      $addr = $_REQUEST['addr'];
      jsonApiCall('/module', array(
        'addr' => $addr,
        'info' => 1
      ), true);
    }


    ?>
    <div id="debug">
      <form action="<?php echo $_SERVER['SCRIPT_NAME']; ?>" method="POST">
        <p>
          <label for="addr">Moduladdresse:</label>
          <input name="addr" id="addr" type="number" value="<?php echo $_REQUEST['addr']; ?>"/>
          <label for="pos">Blattnummer:</label>
          <input name="pos" id="pos" type="number" value="<?php echo $_REQUEST['pos']; ?>"/>
          <button id="setsingle" name="setsingle" type="submit" value="setsingle">Setzen</button>
          <button id="info" name="info" type="submit" value="info">Info</button>
        </p>
        <p>
          <label for="hexbytes">Hexbytes:</label>
          <input name="hexbytes" id="hexbytes" type="text" size="80" value="<?php echo $_REQUEST['hexbytes']; ?>"/>
          <button id="sendhex" name="sendhex" type="submit" value="sendhex">Senden</button>
        </p>


      </form>
    </div>

  </body>

</html>
