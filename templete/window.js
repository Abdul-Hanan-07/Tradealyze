// Connect to the Tradealyze C++ WebSocket server
const ws = new WebSocket('ws://localhost:9002'); 

// ─── INCOMING: Listen for data from C++ ──────────────────────────────────────
ws.onmessage = (event) => {
  try {
    const msg = JSON.parse(event.data);

    switch (msg.type) {
      case 'orderbook':
        window.TRADESIM.updateOrderBook(msg.asks, msg.bids);
        break;
      case 'trade':
        window.TRADESIM.addTransaction(msg.tx);
        break;
      case 'chart':
        window.TRADESIM.updateChart(msg.points); // [{x, y}, ...]
        break;
      case 'balances':
        window.TRADESIM.updateBalances(msg); // { btc, eth, usdt }
        break;
      default:
        console.warn('Unknown message type received from backend:', msg.type);
    }
  } catch (error) {
    console.error('Error parsing WebSocket message:', error);
  }
};

// ─── CONNECTION LIFECYCLE ────────────────────────────────────────────────────
ws.onopen = () => {
  console.log('Connected to Tradealyze C++ Engine');
  window.TRADESIM.setConnected(true);
};

ws.onclose = () => {
  console.log('Disconnected from Tradealyze C++ Engine');
  window.TRADESIM.setConnected(false);
};

ws.onerror = (error) => {
  console.error('WebSocket Error:', error);
};

// ─── OUTGOING: Send data to C++ ──────────────────────────────────────────────

// Hook this into your HTML submitOrder() function
window.TRADESIM.sendOrder = function(orderData) {
  if (ws.readyState === WebSocket.OPEN) {
    const payload = {
      action: "submit_order",
      side: orderData.side,             // 'buy' or 'sell'
      product: orderData.product,       // e.g., 'ETH/BTC'
      price: orderData.price,
      amount: orderData.amount,
      orderType: orderData.orderType    // 'limit', 'market', etc.
    };
    
    ws.send(JSON.stringify(payload));
    console.log('Order sent to C++ backend:', payload);
  } else {
    console.error('Cannot send order: WebSocket is not connected.');
  }
};

// Hook this into your HTML nextTimeframe() function
window.TRADESIM.advanceTimeframeBackend = function() {
  if (ws.readyState === WebSocket.OPEN) {
    const payload = { action: "next_timeframe" };
    ws.send(JSON.stringify(payload));
    console.log('Timeframe advance requested to C++ backend');
  } else {
    console.error('Cannot advance timeframe: WebSocket is not connected.');
  }
};