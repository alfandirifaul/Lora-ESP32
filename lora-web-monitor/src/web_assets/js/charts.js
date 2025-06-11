const ctx = document.getElementById('dataChart').getContext('2d');
let dataChart;

function initializeChart() {
    dataChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [], // Time labels will be added dynamically
            datasets: [{
                label: 'LoRa Data',
                data: [], // Data points will be added dynamically
                borderColor: 'rgba(75, 192, 192, 1)',
                backgroundColor: 'rgba(75, 192, 192, 0.2)',
                borderWidth: 1,
                fill: true,
            }]
        },
        options: {
            responsive: true,
            scales: {
                x: {
                    type: 'time',
                    time: {
                        unit: 'minute'
                    },
                    title: {
                        display: true,
                        text: 'Time'
                    }
                },
                y: {
                    title: {
                        display: true,
                        text: 'Received Data'
                    }
                }
            }
        }
    });
}

function updateChart(time, value) {
    if (dataChart.data.labels.length > 20) { // Limit to 20 data points
        dataChart.data.labels.shift();
        dataChart.data.datasets[0].data.shift();
    }
    dataChart.data.labels.push(time);
    dataChart.data.datasets[0].data.push(value);
    dataChart.update();
}

// WebSocket connection for real-time updates
const socket = new WebSocket('ws://yourserveraddress:port');

socket.onmessage = function(event) {
    const receivedData = JSON.parse(event.data);
    const time = new Date(receivedData.timestamp).toLocaleTimeString();
    const value = receivedData.value;
    updateChart(time, value);
};

window.onload = function() {
    initializeChart();
};