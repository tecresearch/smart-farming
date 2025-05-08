let ws;
let connectBtn = document.getElementById('connectBtn');
let statusDiv = document.getElementById('status');
let receivedDataDiv = document.getElementById('receivedData');

// Function to initialize WebSocket connection
function connectWebSocket() {
  // Create WebSocket connection
  ws = new WebSocket('ws://192.168.1.8:81');  // Connect to the WebSocket server (replace with the actual IP)

  ws.onopen = () => {
    statusDiv.textContent = 'Connected to WebSocket server';
    console.log('WebSocket connection established');
    // Send initial handshake
    ws.send(JSON.stringify({ type: 'handshake', device: 'BrowserClient' }));
  };

  ws.onmessage = (event) => {
    // Handle messages from the server
    console.log('Message received:', event.data);
    receivedDataDiv.textContent = 'Received data: ' + event.data;
  };

  ws.onclose = () => {
    statusDiv.textContent = 'Disconnected from WebSocket server';
    console.log('WebSocket connection closed');
  };

  ws.onerror = (error) => {
    console.log('WebSocket Error:', error);
    statusDiv.textContent = 'Error in WebSocket connection';
  };
}

// Attach event listener to the connect button
connectBtn.addEventListener('click', () => {
  connectWebSocket();
});
