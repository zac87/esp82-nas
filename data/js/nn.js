function validateUpload() {
    if(document.getElementById("uploadFileName").value.trim() == "") {
      return false;
    }
    return true;
  }

  function validateDeleteAll() {
    if(confirm("Do you want to delete all files on your e ?")) {
      return true;
    } else {
      return false;
    }
  }

  function httpReset()
  {
      var xmlHttp = new XMLHttpRequest();
      xmlHttp.open( "GET", "reset", true); // false for synchronous request decrp.
      xmlHttp.send( null );
      return xmlHttp.responseText;
  }

  function httpGet(url, urloptions, callback) {
    var httpRequest = new XMLHttpRequest();
    httpRequest.onload = function(){ // When the request is loaded
       callback(httpRequest.responseText);// We're calling our method
    };

    httpRequest.open('GET', url + urloptions);
    httpRequest.send();
}