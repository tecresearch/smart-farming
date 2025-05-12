require('dotenv').config();
const WebSocket = require('ws');
const express = require('express');
const path = require('path');

// Create Express app
const app = express();
const PORT = process.env.PORT || 3000;

// Serve static files
app.use(express.static(path.join(__dirname, 'public')));

// Create HTTP server (Render will automatically handle HTTPS)
const server = require('http').createServer(app);

// WebSocket server
const wss = new WebSocket.Server({ server });

const clients = new Set();
const sensorData = new Map();

wss.on('connection', (ws) => {
  console.log(`New client connected`);
  clients.add(ws);

  // Send existing sensor data on connect
  sensorData.forEach((data, sensorId) => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ sensorId, ...data }));
    }
  });

  // Heartbeat
  const heartbeatInterval = setInterval(() => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ type: 'heartbeat' }));
    }
  }, 15000);

  // Handle messages
  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message);
      console.log('Received:', data);

      if (data.type === 'heartbeat') return;

      if (data.sensorId) {
        if (!sensorData.has(data.sensorId)) {
          sensorData.set(data.sensorId, {});
        }
        const sensor = sensorData.get(data.sensorId);
        Object.assign(sensor, data);
        sensor.lastUpdated = new Date().toISOString();

        clients.forEach(client => {
          if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify(data));
          }
        });
      }
    } catch (e) {
      console.error('Error processing message:', e);
    }
  });

  ws.on('close', () => {
    console.log('Client disconnected');
    clients.delete(ws);
    clearInterval(heartbeatInterval);
  });

  ws.on('error', (error) => {
    console.error('WebSocket error:', error);
  });
});

// Clean up disconnected clients
setInterval(() => {
  clients.forEach(client => {
    if (client.readyState !== WebSocket.OPEN) {
      clients.delete(client);
    }
  });
}, 30000);

// Start server
server.listen(PORT, '0.0.0.0', () => {
  const isProduction = !!process.env.RENDER;
  console.log(`=================================`);
  console.log(`Server running in ${isProduction ? 'PRODUCTION' : 'DEVELOPMENT'} mode`);
  console.log(`Port: ${PORT}`);
  console.log(`WebSocket endpoint: ${isProduction 
    ? `wss://${process.env.RENDER_EXTERNAL_HOSTNAME}` 
    : `ws://localhost:${PORT}`}`);
  console.log(`HTTP endpoint: ${isProduction 
    ? `https://${process.env.RENDER_EXTERNAL_HOSTNAME}` 
    : `http://localhost:${PORT}`}`);
  console.log(`=================================`);
});
