// generic utilities

function escapehtml(htmltext)
{
  return $("<div>").text(htmltext).html();
}


function formattedAge(age, components)
{
  var s = '';
  dd = [
    { "name":"\"", "f":1 },
    { "name":"'", "f":60 },
    { "name":"h", "f":3600 },
    { "name":"d", "f":86400 },
    { "name":"mt", "f":2628000 }, // approx, 1/12 year
    { "name":"yr", "f":365*86400 }
  ];
  var i= dd.length-1;
  while (i>0) {
    if (age>=dd[i].f) {
      break;
    }
    i--;
  }
  var first = true;
  while (components>0 && i>=0 && age>0) {
    if (!first) s += ' ';
    var v = Math.floor(age/dd[i].f);
    s += v.toString() + dd[i].name;
    age -= v*dd[i].f;
    first = false;
    i--;
    components--;
  }
  return s;
}


function selectText(jqfield, start, end)
{
  var field = jqfield.get(0);
  if( field.createTextRange ) {
    var selRange = field.createTextRange();
    selRange.collapse(true);
    selRange.moveStart('character', start);
    selRange.moveEnd('character', end);
    selRange.select();
  } else if( field.setSelectionRange ) {
    field.setSelectionRange(start, end);
  } else if( field.selectionStart ) {
    field.selectionStart = start;
    field.selectionEnd = end;
  }
  field.focus();
}



function show(element)
{
  $(element).show();
}


function hide(element)
{
  $(element).hide();
}


function showIf(cond, element)
{
  if (cond)
    show(element);
  else
    hide(element);
}


function getTarget(event)
{
  if (!event) return undefined;
  if (event.currentTarget) return event.currentTarget;
  return event.target;
}


function buttonFeedback(target,color)
{
  if (target) {
    if (target.getAttribute('originalStyle')==null) {
      // no feedback already running, start it now
      var currentStyle = target.getAttribute('style')
      if (!currentStyle) currentStyle = '/* none */'
      target.setAttribute('originalStyle', currentStyle)
      target.setAttribute('style', 'background-color: ' + color + ';')
      setTimeout(function() {
        var originalStyle = target.getAttribute('originalStyle')
        if (originalStyle) {
          target.removeAttribute('style')
          if (originalStyle!='/* none */') {
            target.setAttribute('style', originalStyle)
          }
        }
        target.removeAttribute('originalStyle')
      }, 300)
    }
  }
}



function install_longclickables(selector)
{
  // (re)install custom handler which also detects longclick/taphold
  $((selector ? selector+" " : "") + ".p44-longclickable").each(function(i, obj) {
    $(this).off("taphold")
    $(this).off("click")
    $(this).on("click", function(event) {
      // Note: use .currentHandler, not .target, as the latter might be something else from which the event has bubbled up.
      var handler = window[event.currentTarget.getAttribute('p44-handler')];
      var hiddenFeatures = (event.metaKey || event.ctrlKey) ? 2 : 0
      handler(event, hiddenFeatures) // hiddenFeatures arg==2 means actual meta/ctrl keys
    })
    $(this).on("taphold", function(event) {
      var handler = window[event.currentTarget.getAttribute('p44-handler')];
      handler(event, 1) // hiddenFeatures arg==1 means longclick
    })
  })
}


function fracDigitsForResolution(res)
{
  var fracDigits = Math.floor(-Math.log(res)/Math.log(10)+0.99)
  if (fracDigits<0) fracDigits = 0; else if (fracDigits>20) fracDigits = 20
  return fracDigits
}


function roundToFracDigits(value, fracdigits)
{
  let r = Math.pow(10, fracdigits)
  return Math.round(value*r)/r
}


function roundToResolution(value, res)
{
  return roundToFracDigits(value, fracDigitsForResolution(res))
}


