// helper functions to manage JQM dialogs

var currentOpenDialog = false;

function openDialog(dialog, openedCB)
{
  if (currentOpenDialog) {
    // a dialog is still open, first close that, then open the new one
    closeDialog(function() {
      // defer opening!
      setTimeout(function() {
        openDialog(dialog, openedCB);
      }, 10 );
    });
    // done here, callback will cause calling openDialog again
    return;
  }
  $(dialog).on("popupafteropen.dialog", function() {
    $(dialog).off("popupafteropen.dialog");
    if (openedCB!=undefined) {
      openedCB();
    }
  });
  // for closing other than via closeDialog(), such as data-dismissible
  $(dialog).off("popupafterclose.dialog");
  $(dialog).on("popupafterclose.dialog", function() {
    currentOpenDialog = false;
  });
  // make .clickselect and .infovalue elements select their content text on click
  $(dialog).find('.clickselect, .infovalue').each(function() {
    $(this).on("mousedown", function(event) {
      console.log("INFOVALUE MOUSEDOWN, event:" + event.currentTarget.toString());
      event.stopPropagation();
      var range = document.createRange();
      range.selectNodeContents(event.currentTarget);
      window.getSelection().removeAllRanges();
      window.getSelection().addRange(range);
      return false;
    })
  })
  // clear marked fields
  $(dialog).find('input.p44-input-clear').each(function() {
    $(this).val('')
  })
  // prevent super annoying jumps-to-focused when directly clicking a button or link
  // Trick is to prevent mousedown default processing
  $(dialog).find('button').each(function() {
    $(this).attr("onmousedown", "return false;");
  })
  $(dialog).find('a').each(function() {
    $(this).attr("onmousedown", "return false;");
  })
  currentOpenDialog = dialog;
  if ($(dialog).attr('data-dismissible')!==undefined) {
    $(dialog).popup({ positionTo:"window" }); // keep original data-dismissible
  }
  else {
    $(dialog).popup({ positionTo:"window", dismissible:true }); // set global default for data-dismissible
  }
  $(dialog).popup("open");
  //$(dialog).popup("open", { positionTo:"window" });
}


function closeDialog(closedCB)
{
  var dialog = currentOpenDialog;
  currentOpenDialog = false;
  if (dialog!=false) {
    $(dialog).off("popupafterclose.dialog");
    $(dialog).on("popupafterclose.dialog", function() {
      $(dialog).off("popupafterclose.dialog");
      if (closedCB!=undefined) {
        closedCB();
      }
    });
    $(dialog).popup("close");
  }
  else {
    // no dialog open any more, just execute callback
    if (closedCB!=undefined) {
      closedCB();
    }
  }
}
