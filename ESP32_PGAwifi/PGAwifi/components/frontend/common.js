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


function updateWiFiSettings() {
    var ssid = document.getElementById('ssid').value;
    var pwd = document.getElementById('password').value;

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
}


function fetchAvailableNetworks() {
    stopRefreshListInterval();

    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/getAvailableNetworks', true);
    xhr.setRequestHeader('Content-Type', 'application/json');

    xhr.onload = function () {
        if (xhr.status >= 200 && xhr.status < 300) {
            var data = JSON.parse(xhr.responseText);
            var selectElement = document.getElementById('available-networks');
            selectElement.innerHTML = '';

            data.forEach(function (network) {
                var option = document.createElement('option');
                option.value = network.ssid;
                option.text = network.ssid;
                selectElement.add(option);
            });

            startRefreshListInterval();
        } else {
            console.error('Error:', xhr.statusText);
            startRefreshListInterval(); // Ensure interval restarts even in case of an error
        }
    };

    xhr.onerror = function () {
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

fetchAvailableNetworks();
