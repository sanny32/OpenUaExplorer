(function () {
  "use strict";

  function initThemeToggle() {
    var shot = document.getElementById("app-screenshot");
    var darkButtons = document.querySelectorAll(".btn-theme-dark");
    var lightButtons = document.querySelectorAll(".btn-theme-light");

    function setButtonsState(buttons, active) {
      buttons.forEach(function (button) {
        button.classList.toggle("active", active);
        button.setAttribute("aria-pressed", String(active));
      });
    }

    function setTheme(theme) {
      var dark = theme === "dark";
      shot.src = dark ? "assets/app-dark.png" : "assets/app-light.png";
      setButtonsState(darkButtons, dark);
      setButtonsState(lightButtons, !dark);
    }

    darkButtons.forEach(function (button) {
      button.addEventListener("click", function () { setTheme("dark"); });
    });
    lightButtons.forEach(function (button) {
      button.addEventListener("click", function () { setTheme("light"); });
    });

    new Image().src = "assets/app-light.png";
  }

  function initDownloadLinks() {
    var repo = "sanny32/OpenUaExplorer";

    function assetUrl(assets, pattern) {
      for (var i = 0; i < assets.length; i++) {
        if (pattern.test(assets[i].name)) {
          return assets[i].browser_download_url;
        }
      }
      return null;
    }

    function setLink(id, url) {
      if (url) {
        document.getElementById(id).href = url;
      }
    }

    fetch("https://api.github.com/repos/" + repo + "/releases?per_page=10")
      .then(function (response) {
        if (!response.ok) {
          throw new Error("HTTP " + response.status);
        }
        return response.json();
      })
      .then(function (releases) {
        var release = null;
        for (var i = 0; i < releases.length; i++) {
          if (!releases[i].draft) {
            release = releases[i];
            break;
          }
        }
        if (!release) {
          return;
        }

        var assets = release.assets || [];
        var appImage = assetUrl(assets, /\.AppImage$/i);
        setLink("dl-windows", assetUrl(assets, /win64.*\.exe$/i) || assetUrl(assets, /\.exe$/i));
        setLink("dl-linux", appImage);
        setLink("dl-appimage", appImage);
        setLink("dl-deb", assetUrl(assets, /\.deb$/i));
        setLink("dl-rpm", assetUrl(assets, /\.rpm$/i));
        setLink("dl-macos", assetUrl(assets, /\.dmg$/i));

        document.getElementById("release-version").textContent = release.tag_name;
        document.getElementById("release-link").href = release.html_url;
        document.getElementById("release-prerelease").hidden = !release.prerelease;
        document.getElementById("release-info").hidden = false;
      })
      .catch(function () {});
  }

  initThemeToggle();
  initDownloadLinks();
})();
