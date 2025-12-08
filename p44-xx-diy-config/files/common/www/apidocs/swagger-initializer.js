

window.onload = function() {

  // Fetch CSRF token
  var csrfToken = "";
  try {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/tok/json", false); // synchronous for simplicity
    xhr.send(null);
    if (xhr.status === 200) {
      csrfToken = JSON.parse(xhr.responseText);
    }
  } catch (e) {
    console.warn("Failed to fetch CSRF token:", e);
  }

  window.ui = SwaggerUIBundle({
    url: "./p44-json-api.yaml",
    dom_id: '#swagger-ui',
    deepLinking: true,
    presets: [
      SwaggerUIBundle.presets.apis,
      SwaggerUIStandalonePreset
    ],
    plugins: [
      SwaggerUIBundle.plugins.DownloadUrl
    ],
    layout: "BaseLayout",
    // format JSON replies nicely
    responseInterceptor: function (response) {
      try {
        const contentType = response.headers.get("content-type");
        if (contentType && contentType.includes("application/json")) {
          // Parse and re-stringify with indentation
          const parsed = JSON.parse(response.text);
          response.text = JSON.stringify(parsed, null, 2);
        }
      } catch (e) {
        // Do nothing if not valid JSON
      }
      return response;
    },


  });
};
