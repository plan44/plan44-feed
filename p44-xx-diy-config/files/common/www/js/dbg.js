
function getinput(editor, promptmarkerid, getlinenotatend, notpromtattop)
{
  var session = editor.getSession()
  var sel = session.getSelection()
  var currentrow = editor.getSelectionRange().start.row
  var input = {}
  if (!sel.isEmpty()) {
    // we have a selection, use that
    input.text = session.getTextRange(editor.getSelectionRange())
  }
  else {
    // nothing selected
    var startrow = null
    var endrow = session.getLength()
    if (promptmarkerid) {
      // use text from line following prompt marker and end of session
      startrow = session.getMarkers()[promptmarkerid].range.start.row+1
      if (currentrow<startrow) endrow = null // cursor is before prompt marker
    }
    if (!startrow) {
      startrow = currentrow
      if (startrow+1<endrow) endrow = null // not at end of text
    }
    //console.log(`currentrow=${currentrow}, startrow=${startrow}, endrow=${endrow}, lastrow=${session.getLength()}`)
    if (!endrow && getlinenotatend) {
      // just copy current line
      input.text = session.getLine(currentrow)
      endrow = currentrow
    }
    else if (endrow && (!notpromtattop || startrow!=1)) {
      var lines = session.getLines(startrow, endrow)
      input.text = ''
      var sep = ''
      for (var i=0; i<lines.length; i++) {
        input.text += sep + lines[i]
        sep = "\n"
      }
    }
    else {
      //console.log(`no input`)
      return false;
    }
  }
  input.atend = endrow>=session.getLength()
  //console.log(`input.text=${input.text}, input.atend=${input.atend}`)
  return input
}



function panelShown(panel)
{
  return !$(panel).hasClass('collapse')
}

var gLastPanelHeight = '60vh';

function togglePanel(panel, otherpanel, show)
{
  var nowshown = panelShown(panel)
  if (show===undefined) show = !nowshown
  if (show!=nowshown) {
    if (nowshown) {
      // shown now -> hide
      gLastPanelHeight = $(otherpanel).css('height');
      $(otherpanel).css('height', '100%') // release from fixed height set by js-resizable
      $(panel).addClass('collapse');
    }
    else {
      // hidden now -> show
      $(otherpanel).css('height', gLastPanelHeight); // restore height
      $(panel).removeClass('collapse')
    }
    return true // changed
  }
  return false // not changed
}




