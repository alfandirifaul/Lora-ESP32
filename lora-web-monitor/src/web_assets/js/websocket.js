const socket = new WebSocket('ws://your-server-address:port');

socket.onopen = function() {
    console.log('WebSocket connection established');
};

socket.onmessage = function(event) {
    const data = JSON.parse(event.data);
    updateStatus(data.status);
    logData(data.log);
};

socket.onclose = function() {
    console.log('WebSocket connection closed');
};

function updateStatus(status) {
    const statusElement = document.getElementById('status');
    statusElement.innerText = `LoRa Status: ${status}`;
}

function logData(logEntry) {
    const logElement = document.getElementById('log');
    const newLogEntry = document.createElement('div');
    newLogEntry.innerText = logEntry;
    logElement.appendChild(newLogEntry);
}