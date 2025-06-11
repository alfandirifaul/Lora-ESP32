// This file contains the main JavaScript functionality for the web pages.

document.addEventListener("DOMContentLoaded", function() {
    const statusButton = document.getElementById("status-button");
    const logButton = document.getElementById("log-button");
    const wifiButton = document.getElementById("wifi-button");

    statusButton.addEventListener("click", function() {
        fetchStatus();
    });

    logButton.addEventListener("click", function() {
        fetchLogs();
    });

    wifiButton.addEventListener("click", function() {
        fetchWiFiConfig();
    });

    function fetchStatus() {
        fetch('/status')
            .then(response => response.json())
            .then(data => {
                document.getElementById("status-display").innerText = JSON.stringify(data, null, 2);
            })
            .catch(error => console.error('Error fetching status:', error));
    }

    function fetchLogs() {
        fetch('/logs')
            .then(response => response.json())
            .then(data => {
                const logDisplay = document.getElementById("log-display");
                logDisplay.innerHTML = '';
                data.forEach(log => {
                    const logEntry = document.createElement("div");
                    logEntry.innerText = log;
                    logDisplay.appendChild(logEntry);
                });
            })
            .catch(error => console.error('Error fetching logs:', error));
    }

    function fetchWiFiConfig() {
        fetch('/wifi-config')
            .then(response => response.json())
            .then(data => {
                document.getElementById("wifi-display").innerText = JSON.stringify(data, null, 2);
            })
            .catch(error => console.error('Error fetching WiFi config:', error));
    }
});