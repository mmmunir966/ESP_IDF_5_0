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

async function updateWiFiSettings() {
    var ssid = document.getElementById('ssid').value;
    var pwd = document.getElementById('password').value;

    await fetch('/updateWiFi', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
            'ssid': ssid,
            'password': pwd,
        },
    })
        .then(() => {
            console.log('WiFi settings updated successfully.');

            document.getElementById('ssid').value = '';
            document.getElementById('password').value = '';
        })
        .catch((error) => {
            console.error('Error:', error);
        });
}

async function fetchAvailableNetworks() {
    stopRefreshListInterval();
    await fetch('/getAvailableNetworks')
        .then(response => response.json())
        .then(data => {
            var selectElement = document.getElementById('available-networks');
            selectElement.innerHTML = '';
            data.forEach(network => {
                var option = document.createElement('option');
                option.value = network.ssid;
                option.text = network.ssid;
                selectElement.add(option);
            });
        })
        .catch((error) => {
            console.error('Error:', error);
        });
    startRefreshListInterval();
}

function updateSSIDField() {
    var selectElement = document.getElementById('available-networks');
    var ssidField = document.getElementById('ssid');
    ssidField.value = selectElement.value;
}

fetchAvailableNetworks();
