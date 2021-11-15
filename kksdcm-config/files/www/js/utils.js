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


function buttonFeedback(event,color)
{
  if (event && event.target) {
    var oldStyle = event.target.getAttribute('style');
    event.target.setAttribute('style', 'background-color: ' + color + ';');
    setTimeout(function() {
      event.target.removeAttribute('style');
      if (oldStyle) event.target.setAttribute('style', oldStyle);
    }, 300)
  }
}
