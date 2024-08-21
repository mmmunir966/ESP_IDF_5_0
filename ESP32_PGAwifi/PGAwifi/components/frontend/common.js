var refreshListIntervalValue = 10000;
var refreshListInterval = null;

function startRefreshListInterval() {
    refreshListInterval = setInterval(fetchAvailableNetworks, refreshListIntervalValue);
}

function stopRefreshListInterval() {
    if (refreshListInterval != null) {
        clearInterval(refreshListInterval);
        refreshListInterval = null;
    }
}

function showCredentialUpdatePopup() {
    // Create the popup element
    const popup = document.createElement("div");
    popup.classList.add("credentials-updated"); // Add CSS class for styling

    // Set popup styles (replace with your desired styles)
    popup.style.position = "fixed";
    popup.style.top = 0;
    popup.style.left = 0;
    popup.style.right = 0;
    popup.style.bottom = 0;
    popup.style.backgroundColor = "rgba(0, 0, 0, 0.5)";
    popup.style.zIndex = 9999;

    // Create the inner content container
    const contentContainer = document.createElement("div");
    contentContainer.style.position = "absolute";
    contentContainer.style.top = "50%";
    contentContainer.style.left = "50%";
    contentContainer.style.transform = "translate(-50%, -50%)";
    contentContainer.style.backgroundColor = "white";
    contentContainer.style.padding = "20px";
    contentContainer.style.borderRadius = "5px";
    contentContainer.style.textAlign = "center";

    // Create the message paragraph
    const message = document.createElement("p");
    message.textContent = "WiFi Credentials are updated.";

    // Create the close button
    const closeButton = document.createElement("button");
    closeButton.textContent = "OK";
    closeButton.style.padding = "10px 20px";
    closeButton.style.border = "none";
    closeButton.style.backgroundColor = "#007bff";
    closeButton.style.color = "white";
    closeButton.style.borderRadius = "5px";
    closeButton.style.cursor = "pointer";

    // Close button click event listener
    closeButton.addEventListener("click", function () {
        document.body.removeChild(popup);
    });

    // Add elements to the popup
    contentContainer.appendChild(message);
    contentContainer.appendChild(closeButton);
    popup.appendChild(contentContainer);

    // Add the popup to the body
    document.body.appendChild(popup);
}

function showEmptyCredentialPopup() {
    // Create the popup element
    const popup = document.createElement("div");
    popup.classList.add("credentials-updated"); // Add CSS class for styling

    // Set popup styles (replace with your desired styles)
    popup.style.position = "fixed";
    popup.style.top = 0;
    popup.style.left = 0;
    popup.style.right = 0;
    popup.style.bottom = 0;
    popup.style.backgroundColor = "rgba(0, 0, 0, 0.5)";
    popup.style.zIndex = 9999;

    // Create the inner content container
    const contentContainer = document.createElement("div");
    contentContainer.style.position = "absolute";
    contentContainer.style.top = "50%";
    contentContainer.style.left = "50%";
    contentContainer.style.transform = "translate(-50%, -50%)";
    contentContainer.style.backgroundColor = "white";
    contentContainer.style.padding = "20px";
    contentContainer.style.borderRadius = "5px";
    contentContainer.style.textAlign = "center";

    // Create the message paragraph
    const message = document.createElement("p");
    message.textContent = "WiFi Network is not selected.";

    // Create the close button
    const closeButton = document.createElement("button");
    closeButton.textContent = "OK";
    closeButton.style.padding = "10px 20px";
    closeButton.style.border = "none";
    closeButton.style.backgroundColor = "#007bff";
    closeButton.style.color = "white";
    closeButton.style.borderRadius = "5px";
    closeButton.style.cursor = "pointer";

    // Close button click event listener
    closeButton.addEventListener("click", function () {
        document.body.removeChild(popup);
    });

    // Add elements to the popup
    contentContainer.appendChild(message);
    contentContainer.appendChild(closeButton);
    popup.appendChild(contentContainer);

    // Add the popup to the body
    document.body.appendChild(popup);
}

function updateWiFiSettings() {
    var ssid = document.getElementById('ssid').value;
    var pwd = document.getElementById('password').value;
    if (ssid == '') {
        showEmptyCredentialPopup();
    }
    else {
        var xhr = new XMLHttpRequest();
        xhr.open('POST', '/updateWiFi', true);
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.setRequestHeader('ssid', ssid);
        xhr.setRequestHeader('password', pwd);

        xhr.onload = function () {
            if (xhr.status >= 200 && xhr.status < 300) {
                var data = JSON.parse(xhr.responseText);
                console.log('WiFi settings updated successfully.');

                document.getElementById('ssid').value = '';
                document.getElementById('password').value = '';
            } else {
                console.error('Error:', xhr.statusText);
            }
        };

        xhr.onerror = function () {
            console.error('Request failed');
        };

        xhr.send();

        showCredentialUpdatePopup();
    }
}

// Helper function to get the previously selected SSID (implementation depends on your storage mechanism)
function getPreviouslySelectedSsid() {
    // Replace this with your logic to retrieve the last selected SSID from localStorage, cookies, or a database
    var storedSsid = document.getElementById('ssid').value;
    return storedSsid || ''; // Return empty string if no SSID is found
}

function fetchAvailableNetworks() {
    stopRefreshListInterval();

    var xhr = new XMLHttpRequest();
    xhr.open('GET', 'getAvailableNetworks', true);

    xhr.onload = function() {
        if (xhr.status >= 200 && xhr.status < 300) {
            console.log('Response received:', xhr.responseText); // Log the response

            try {
                var data = JSON.parse(xhr.responseText);
                var selectElement = document.getElementById('available-networks');
                if (!selectElement) {
                    console.error('No element with id "available-networks" found.');
                    startRefreshListInterval();
                    return;
                }
                selectElement.innerHTML = '';

                var lastSelectedSsid = getPreviouslySelectedSsid(); // Get the last selected SSID

                var option = document.createElement('option');
                option.value = '';
                option.text = '<Select>';
                selectElement.add(option);

                data.forEach(function(network) {
                    var option = document.createElement('option');
                    option.value = network.ssid;
                    option.text = network.ssid;
                    selectElement.add(option);

                    // Set the selected attribute if it matches the last selected SSID
                    if (lastSelectedSsid === network.ssid) {
                        option.selected = true;
                    }
                });

                startRefreshListInterval();
            } catch (e) {
                console.error('JSON Parse Error:', e);
            }
        } else {
            console.error('Error:', xhr.statusText);
            startRefreshListInterval(); // Ensure interval restarts even in case of an error
        }
    };

    xhr.onerror = function() {
        console.error('Request failed');
        startRefreshListInterval(); // Ensure interval restarts even in case of an error
    };

    xhr.send();
}


function updateSSIDField() {
    var selectElement = document.getElementById('available-networks');
    var ssidField = document.getElementById('ssid');
    ssidField.value = selectElement.value;
}

// fetchAvailableNetworks();
document.addEventListener('DOMContentLoaded', function() {
    fetchAvailableNetworks();
});
