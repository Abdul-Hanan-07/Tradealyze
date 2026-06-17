// ─── GLOBAL STATE ────────────────────────────────────────────────────────────
const STATE = {
  side: 'buy', orderType: 'limit', timeframe: 0,
  sessionTrades: 0, sessionVol: 0, simTick: 0,
  transactions: [], chartPts: [], basePrice: 0.03421,
  activeView: 'dashboard',
  bestBid: null,   // populated by price_hint from backend
  bestAsk: null,   // populated by price_hint from backend
  pendingOrders: {}, // tracks optimistic orders by key so we can update their status
};
const TF_LABELS = ['T+0 (Genesis)','T+1 (Morning)','T+2 (Midday)','T+3 (Afternoon)','T+4 (Close)','T+5 (After Hours)'];

// ─── VIEW SWITCHING (preserves WS connection) ────────────────────────────────
function switchView(name) {
  document.querySelectorAll('.view').forEach(v => v.classList.remove('active'));
  document.getElementById('view-' + name).classList.add('active');
  document.querySelectorAll('.nav-tab').forEach(t => t.classList.remove('active'));
  event.currentTarget.classList.add('active');
  STATE.activeView = name;
}

// ─── WEBSOCKET ────────────────────────────────────────────────────────────────
const ws = new WebSocket('ws://localhost:9002');

ws.onopen = () => {
  const el = document.getElementById('conn-status');
  el.textContent = '● CONNECTED';
  el.className = 'nav-tag connected-badge';
};
ws.onclose = () => {
  const el = document.getElementById('conn-status');
  el.textContent = '○ DISCONNECTED';
  el.className = 'nav-tag';
};
ws.onmessage = (event) => {
  try {
    const msg = JSON.parse(event.data);
    switch (msg.type) {
      case 'balances':   updateBalances(msg); break;
      case 'orderbook':  updateOrderBook(msg.asks, msg.bids); break;
      case 'trade':      addTransaction(msg.tx); break;
      case 'chart':      updateChart(msg.points); break;
      case 'products':   updateMarkets(msg.products); break;
      case 'order_ack':  handleOrderAck(msg); break;
      case 'price_hint':
        STATE.bestBid = msg.best_bid;
        STATE.bestAsk = msg.best_ask;
        break;
    }
  } catch(e) { console.error('WS parse error', e); }
};

// ─── BACKEND HOOKS ────────────────────────────────────────────────────────────
function updateBalances({ btc, eth, usdt, avax }) {
  if (btc  !== undefined) document.getElementById('bal-btc').textContent  = parseFloat(btc).toFixed(5);
  if (eth  !== undefined) document.getElementById('bal-eth').textContent  = parseFloat(eth).toFixed(4);
  if (usdt !== undefined) document.getElementById('bal-usdt').textContent = parseFloat(usdt).toFixed(2);
}

function updateOrderBook(asks, bids) {
  const maxA = asks.length ? parseFloat(asks[asks.length-1].total) : 1;
  const maxB = bids.length ? parseFloat(bids[bids.length-1].total) : 1;
  document.getElementById('asks-list').innerHTML = [...asks].reverse().map(r => {
    const pct = (parseFloat(r.total)/maxA*100).toFixed(1);
    return `<div class="ob-row"><span class="ob-price ask-price">${r.price}</span><span class="ob-amount">${r.amount}</span><span class="ob-total">${r.total}</span><div class="depth-bar ask-bar" style="width:${pct}%"></div></div>`;
  }).join('');
  document.getElementById('bids-list').innerHTML = bids.map(r => {
    const pct = (parseFloat(r.total)/maxB*100).toFixed(1);
    return `<div class="ob-row"><span class="ob-price bid-price">${r.price}</span><span class="ob-amount">${r.amount}</span><span class="ob-total">${r.total}</span><div class="depth-bar bid-bar" style="width:${pct}%"></div></div>`;
  }).join('');
  if (asks.length && bids.length) {
    const spread = (parseFloat(asks[0].price) - parseFloat(bids[0].price)).toFixed(5);
    const pct = (spread / STATE.basePrice * 100).toFixed(3);
    document.getElementById('spread-val').textContent = spread;
    document.getElementById('spread-pct').textContent = `(${pct}%)`;
  }
}

function addTransaction(tx) {
  const sideNorm = String(tx.side).toLowerCase();
  const price = typeof tx.price === 'number' ? tx.price.toFixed(5) : String(tx.price);
  const amount = typeof tx.amount === 'number' ? tx.amount.toFixed(4) : String(tx.amount);

  const existing = STATE.transactions.find(t =>
    String(t.side).toLowerCase() === sideNorm &&
    t.product === tx.product &&
    String(t.status) === 'Pending'
  );

  if (existing) {
    existing.status = tx.status || 'Filled';
    existing.price  = price;
    existing.amount = amount;
    existing.total  = tx.total;
  } else {
    STATE.transactions.unshift({ ...tx, side: sideNorm, price, amount });
    if (STATE.transactions.length > 50) STATE.transactions.pop();
  }
  renderTxHistory();
}

function updateChart(points) {
  STATE.chartPts = points;
  renderChart();
}

function handleOrderAck(msg) {
  const key = `${msg.side}-${msg.product}-${parseFloat(msg.price).toFixed(5)}-${parseFloat(msg.amount).toFixed(4)}`;
  const tx = STATE.pendingOrders[key];
  if (tx) {
    tx.status = msg.status; 
    delete STATE.pendingOrders[key];
    renderTxHistory();
  }
  if (msg.status === 'Rejected ✗') {
    const fb = document.getElementById('order-feedback');
    fb.style.color = 'var(--red)';
    fb.textContent = `✗ Order REJECTED — insufficient balance for ${msg.side.toUpperCase()} ${msg.amount} ${msg.product}`;
    setTimeout(() => fb.textContent = '', 4000);
  }
}

let marketsData = [];
let currentFilter = 'all';
let currentSort = { key: 'pair', dir: 1 };

// ============================================================================
// FIX: MERGE BACKEND PAIRS WITH UI PLACEHOLDERS SO SEARCH/TABS NEVER BREAK
// ============================================================================
function updateMarkets(productsArray) {
  if (productsArray && productsArray.length > 0 && typeof productsArray[0] === 'string') {
    // Start with our complete layout array so all tabs work
    let fullMarketList = [...PLACEHOLDER_MARKETS];
    
    // Ensure any extra custom pair from backend CSV is also appended gracefully
    productsArray.forEach(pairString => {
      if (!fullMarketList.some(m => m.pair === pairString)) {
        fullMarketList.push({
          pair: pairString,
          base: pairString.split('/')[0],
          price: 0, change: 0, high: 0, low: 0, volume: 0, history: []
        });
      }
    });
    marketsData = fullMarketList;
  } else {
    marketsData = productsArray || PLACEHOLDER_MARKETS;
  }
  renderMarkets();
}
// ============================================================================

// ─── SIMULATED ORDER BOOK ────────────────────────────────────────────────────
function genOrderBook(mid) {
  const asks = [], bids = [];
  let cumA = 0, cumB = 0;
  for (let i = 0; i < 8; i++) {
    const p = (mid + (i+1)*0.00003 + Math.random()*0.00005).toFixed(5);
    const amt = (Math.random()*4+0.2).toFixed(4);
    cumA += parseFloat(amt);
    asks.push({ price:p, amount:amt, total:cumA.toFixed(4) });
  }
  for (let i = 0; i < 8; i++) {
    const p = (mid - (i+1)*0.00003 - Math.random()*0.00005).toFixed(5);
    const amt = (Math.random()*4+0.2).toFixed(4);
    cumB += parseFloat(amt);
    bids.push({ price:p, amount:amt, total:cumB.toFixed(4) });
  }
  return { asks, bids };
}

// ─── TICKER & CHART ───────────────────────────────────────────────────────────
function updateTicker(price, prev) {
  const el = document.getElementById('ticker-price');
  const chEl = document.getElementById('ticker-change');
  const pct = ((price-prev)/prev*100).toFixed(2);
  el.textContent = price.toFixed(5);
  el.style.color = price >= prev ? 'var(--green)' : 'var(--red)';
  if (pct >= 0) { chEl.textContent = `+${pct}%`; chEl.className = 'ticker-change tick-up'; chEl.style.cssText = ''; }
  else { chEl.textContent = `${pct}%`; chEl.className = 'ticker-change'; chEl.style.background='rgba(240,79,90,0.12)'; chEl.style.color='var(--red)'; }
  document.getElementById('t-last').textContent = price.toFixed(5);
}

function renderChart() {
  const pts = STATE.chartPts;
  if (pts.length < 2) return;
  const svg = document.querySelector('.chart-grid');
  const w = svg.clientWidth || 500, h = svg.clientHeight || 180;
  const prices = pts.map(p=>p.y);
  const minP = Math.min(...prices), maxP = Math.max(...prices);
  const range = maxP - minP || 0.0001;
  const map = (v,i) => {
    const x = (i/(pts.length-1))*w;
    const y = h - 24 - ((v-minP)/range)*(h-48);
    return `${x.toFixed(1)},${y.toFixed(1)}`;
  };
  const line = prices.map((v,i)=>map(v,i)).join(' ');
  const priceLine = document.getElementById('price-line');
  const priceFill = document.getElementById('price-fill');
  if (priceLine) priceLine.setAttribute('points', line);
  if (priceFill) priceFill.setAttribute('points', `0,${h} ${line} ${w},${h}`);
}

// ─── ORDER FORM ───────────────────────────────────────────────────────────────
function setOrderSide(side) {
  STATE.side = side;
  document.getElementById('btn-buy').className = 'toggle-btn' + (side==='buy'?' active-buy':'');
  document.getElementById('btn-sell').className = 'toggle-btn' + (side==='sell'?' active-sell':'');
  const btn = document.getElementById('submit-btn');
  btn.className = 'submit-btn ' + (side==='buy'?'submit-buy':'submit-sell');
  btn.textContent = side==='buy' ? 'Place Buy Order' : 'Place Sell Order';
  document.getElementById('avail-label').textContent = side==='buy' ? 'Available (USDT)' : 'Available (ETH)';

  if (STATE.orderType !== 'market') {
    const priceInput = document.getElementById('input-price');
    if (side === 'sell' && STATE.bestBid) {
      priceInput.value = parseFloat(STATE.bestBid).toFixed(5);
      const hint = document.getElementById('order-feedback');
      hint.style.color = 'var(--amber)';
      hint.textContent = `ℹ Auto-filled best bid ${parseFloat(STATE.bestBid).toFixed(5)} — your ask must be ≤ this to match`;
      setTimeout(() => hint.textContent = '', 4000);
    } else if (side === 'buy' && STATE.bestAsk) {
      priceInput.value = parseFloat(STATE.bestAsk).toFixed(5);
    }
    calcTotal();
  }
}

function setOrderType(type) {
  STATE.orderType = type;
  ['limit','market','stop'].forEach(t => {
    document.getElementById('type-'+t).className = 'type-chip'+(t===type?' active':'');
  });
  const pi = document.getElementById('input-price');
  pi.disabled = type === 'market';
  pi.placeholder = type === 'market' ? 'Market Price' : '0.03421';
  if (type === 'market') pi.value = '';
}

function updateFormBadge() {
  const v = document.getElementById('input-product').value.trim().toUpperCase() || 'ETH/BTC';
  document.getElementById('form-pair-badge').textContent = v;
  document.getElementById('ticker-pair').textContent = v;

  if (ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify({ action: 'change_market', product: v }));
  }

  STATE.bestBid = null;
  STATE.bestAsk = null;
  STATE.chartPts = [];
  document.getElementById('input-price').value = ''; 
  document.getElementById('est-total').textContent = '—';
  
  document.getElementById('asks-list').innerHTML = '<div style="text-align:center; padding:10px; color:var(--text-secondary)">Loading Market Data...</div>';
  document.getElementById('bids-list').innerHTML = '';
  
  const lineEl = document.getElementById('price-line');
  const fillEl = document.getElementById('price-fill');
  if (lineEl) lineEl.setAttribute('points', '');
  if (fillEl) fillEl.setAttribute('points', '');
}

function calcTotal() {
  const p = parseFloat(document.getElementById('input-price').value);
  const a = parseFloat(document.getElementById('input-amount').value);
  const el = document.getElementById('est-total');
  el.textContent = (!isNaN(p)&&!isNaN(a)&&p>0&&a>0) ? (p*a).toFixed(6) : '—';
}

function fillPct(pct) {
  document.getElementById('input-amount').value = (Math.random()*2*(pct/100)).toFixed(4);
  calcTotal();
}

function submitOrder() {
  const product = document.getElementById('input-product').value.trim() || 'ETH/BTC';
  const priceRaw = STATE.orderType==='market' ? STATE.basePrice : parseFloat(document.getElementById('input-price').value);
  const amount = parseFloat(document.getElementById('input-amount').value);
  const fb = document.getElementById('order-feedback');
  if (isNaN(priceRaw)||isNaN(amount)||amount<=0) {
    fb.style.color='var(--red)'; fb.textContent='⚠ Enter a valid price and amount.'; return;
  }
  const total = (priceRaw*amount).toFixed(6);

  const tx = { time:new Date().toTimeString().slice(0,8), side:STATE.side, product, price:priceRaw.toFixed(5), amount:amount.toFixed(4), total, status:'Pending' };
  STATE.transactions.unshift(tx);
  if (STATE.transactions.length>50) STATE.transactions.pop();
  STATE.sessionTrades++;
  STATE.sessionVol += parseFloat(total);
  document.getElementById('session-trades').textContent = STATE.sessionTrades;
  document.getElementById('session-vol').textContent = STATE.sessionVol.toFixed(5);

  const key = `${STATE.side}-${product}-${priceRaw.toFixed(5)}-${amount.toFixed(4)}`;
  STATE.pendingOrders[key] = tx;

  renderTxHistory();
  fb.style.color = STATE.side==='buy'?'var(--green)':'var(--red)';
  fb.textContent = `✓ ${STATE.side.toUpperCase()} ${amount.toFixed(4)} @ ${priceRaw.toFixed(5)} → sent to backend`;
  if (ws.readyState===WebSocket.OPEN) {
    ws.send(JSON.stringify({ action:'submit_order', side:STATE.side, product, price:priceRaw, amount, orderType:STATE.orderType }));
  } else {
    tx.status = 'No Connection';
    renderTxHistory();
  }
  setTimeout(()=>fb.textContent='', 3500);
}

// ─── TX HISTORY ───────────────────────────────────────────────────────────────
function statusColor(status) {
  if (!status) return 'var(--text-secondary)';
  if (status === 'Filled')       return 'var(--green)';
  if (status === 'Pending')      return 'var(--amber)';
  if (status.startsWith('Rej'))  return 'var(--red)';
  return 'var(--text-secondary)';
}

function renderTxHistory() {
  const rows = STATE.transactions.slice(0,12).map(tx => {
    const b = tx.side==='buy' || tx.side==='BUY';
    return `<tr>
      <td>${tx.time}</td>
      <td class="${b?'td-side-buy':'td-side-sell'}">${String(tx.side).toUpperCase()}</td>
      <td>${tx.product}</td>
      <td class="${b?'td-price-up':'td-price-down'}">${typeof tx.price==='number'?tx.price.toFixed(5):tx.price}</td>
      <td>${typeof tx.amount==='number'?tx.amount.toFixed(4):tx.amount}</td>
      <td>${typeof tx.total==='number'?tx.total.toFixed(6):tx.total}</td>
      <td style="color:${statusColor(tx.status)}">${tx.status||'—'}</td>
    </tr>`;
  }).join('');
  document.getElementById('tx-body').innerHTML = rows || '<tr><td colspan="7" style="color:var(--text-secondary);text-align:center;padding:16px">No transactions yet — place an order above.</td></tr>';
}

function nextTimeframe() {
  STATE.timeframe = (STATE.timeframe+1)%TF_LABELS.length;
  document.getElementById('tf-label').textContent = TF_LABELS[STATE.timeframe];
  if (ws.readyState===WebSocket.OPEN) ws.send(JSON.stringify({action:'next_timeframe'}));
}

// ─── SIM TICK ─────────────────────────────────────────────────────────────────
function simTick() {
  const prev = STATE.basePrice;
  STATE.basePrice = parseFloat((prev*(1+(Math.random()-0.499)*0.003)).toFixed(5));
  STATE.simTick++;
  document.getElementById('sim-time').textContent = `SIM T+${STATE.simTick}`;
  updateTicker(STATE.basePrice, prev);
  const { asks, bids } = genOrderBook(STATE.basePrice);
  updateOrderBook(asks, bids);
  STATE.chartPts.push({ y: STATE.basePrice });
  if (STATE.chartPts.length>80) STATE.chartPts.shift();
  renderChart();
}

// ─── MARKETS TABLE ────────────────────────────────────────────────────────────
const PLACEHOLDER_MARKETS = [
  { pair:'ETH/BTC',  base:'Ethereum',  price:0.03421, change:+1.24, high:0.03490, low:0.03318, volume:128400, history:[0.0338,0.0334,0.0336,0.0340,0.0338,0.0342,0.0345,0.0342] },
  { pair:'DOGE/BTC', base:'Dogecoin',  price:0.000002541, change:-0.87, high:0.000002690, low:0.000002480, volume:8540000, history:[0.0000027,0.00000268,0.00000265,0.00000258,0.00000255,0.00000254,0.00000256,0.00000254] },
  { pair:'LTC/BTC',  base:'Litecoin',  price:0.002841, change:+0.45, high:0.002900, low:0.002790, volume:42100, history:[0.00282,0.00283,0.00281,0.00284,0.00285,0.00283,0.00285,0.00284] },
  { pair:'XRP/BTC',  base:'Ripple',    price:0.0000148, change:+2.11, high:0.0000153, low:0.0000141, volume:3200000, history:[0.0000142,0.0000143,0.0000145,0.0000146,0.0000147,0.0000149,0.0000151,0.0000148] },
  { pair:'ADA/BTC',  base:'Cardano',   price:0.0000110, change:-1.30, high:0.0000118, low:0.0000108, volume:1890000, history:[0.0000115,0.0000113,0.0000112,0.0000111,0.0000110,0.0000109,0.0000110,0.0000110] },
  { pair:'SOL/BTC',  base:'Solana',    price:0.002120, change:+3.54, high:0.002200, low:0.002020, volume:56800, history:[0.00204,0.00206,0.00208,0.00210,0.00209,0.00211,0.00213,0.00212] },
  { pair:'BNB/BTC',  base:'BNB',       price:0.008941, change:+0.88, high:0.009100, low:0.008800, volume:18400, history:[0.00885,0.00888,0.00887,0.00890,0.00891,0.00892,0.00894,0.00894] },
  { pair:'AVAX/BTC', base:'Avalanche', price:0.000472, change:-0.34, high:0.000490, low:0.000465, volume:34200, history:[0.000476,0.000474,0.000473,0.000471,0.000472,0.000470,0.000472,0.000472] },
  { pair:'ETH/USDT', base:'Ethereum',  price:2184.50, change:+1.10, high:2210.00, low:2148.00, volume:98200000, history:[2155,2162,2170,2175,2180,2182,2185,2184] },
  { pair:'BTC/USDT', base:'Bitcoin',   price:63840.00, change:+0.62, high:64200.00, low:63100.00, volume:420000000, history:[63200,63400,63500,63600,63700,63750,63820,63840] },
];
marketsData = PLACEHOLDER_MARKETS;

const PAIR_COLORS = { ETH:'#627eea', BTC:'#f7931a', DOGE:'#c2a633', LTC:'#bebebe', XRP:'#346aa9', ADA:'#3cc8c8', SOL:'#9945ff', BNB:'#f0b90b', AVAX:'#e84142' };

function getPairColor(pair) {
  const base = pair.split('/')[0];
  return PAIR_COLORS[base] || '#3d9bff';
}

function miniSparkline(history, up) {
  if (!history || history.length < 2) return '<svg class="sparkline"><line x1="0" y1="14" x2="80" y2="14" stroke="#1e2d45" stroke-width="1.5"/></svg>';
  const min = Math.min(...history), max = Math.max(...history);
  const range = max - min || 0.001;
  const pts = history.map((v,i) => `${(i/(history.length-1)*78+1).toFixed(1)},${(26 - ((v-min)/range)*22).toFixed(1)}`).join(' ');
  const color = up ? 'var(--green)' : 'var(--red)';
  return `<svg class="sparkline" viewBox="0 0 80 28" xmlns="http://www.w3.org/2000/svg"><polyline points="${pts}" fill="none" stroke="${color}" stroke-width="1.5" stroke-linejoin="round"/></svg>`;
}

function renderMarkets() {
  const query = (document.getElementById('market-search')?.value||'').toLowerCase();
  const maxVol = Math.max(...marketsData.map(m=>m.volume));

  let data = marketsData.filter(m => {
    const matchFilter = currentFilter==='all' || m.pair.includes(currentFilter);
    const matchSearch = m.pair.toLowerCase().includes(query) || (m.base||'').toLowerCase().includes(query);
    return matchFilter && matchSearch;
  });

  data.sort((a,b) => {
    let va=a[currentSort.key], vb=b[currentSort.key];
    if (typeof va==='string') return va.localeCompare(vb)*currentSort.dir;
    return (va-vb)*currentSort.dir;
  });

  const rows = data.map((m,i) => {
    const up = m.change >= 0;
    const color = getPairColor(m.pair);
    const barPct = Math.round(m.volume/maxVol*100);
    return `<tr>
      <td><div class="pair-cell">
        <div class="pair-icon" style="background:${color}22;color:${color}">${m.pair.split('/')[0].slice(0,3)}</div>
        <div><div class="pair-name">${m.pair}</div><div class="pair-base">${m.base||''}</div></div>
      </div></td>
      <td><span class="m-price">${typeof m.price==='number'?m.price.toPrecision(5):m.price}</span></td>
      <td><span class="${up?'m-chg-up':'m-chg-dn'}">${up?'+':''}${m.change.toFixed(2)}%</span></td>
      <td><span class="m-stat">${typeof m.high==='number'?m.high.toPrecision(5):m.high}</span></td>
      <td><span class="m-stat">${typeof m.low==='number'?m.low.toPrecision(5):m.low}</span></td>
      <td><div class="vol-bar-wrap"><span class="m-stat">${m.volume.toLocaleString()}</span><div class="vol-bar-bg"><div class="vol-bar-fill" style="width:${barPct}%"></div></div></div></td>
      <td class="sparkline-cell">${miniSparkline(m.history, up)}</td>
      <td><button class="trade-now-btn" onclick="tradeNow('${m.pair}')">Trade Now →</button></td>
    </tr>`;
  }).join('');

  document.getElementById('markets-body').innerHTML = rows || '<tr><td colspan="8" style="text-align:center;padding:24px;color:var(--text-secondary)">No pairs match your filter.</td></tr>';
}

function filterMarkets(filter, btn) {
  currentFilter = filter;
  if (btn) {
    document.querySelectorAll('.filter-chip').forEach(c=>c.classList.remove('active'));
    btn.classList.add('active');
  }
  renderMarkets();
}

function sortMarkets(key) {
  if (currentSort.key===key) currentSort.dir *= -1;
  else { currentSort.key=key; currentSort.dir=1; }
  document.querySelectorAll('.markets-table th').forEach(th=>th.classList.remove('sort-active'));
  event.currentTarget.classList.add('sort-active');
  renderMarkets();
}

function tradeNow(pair) {
  document.getElementById('input-product').value = pair;
  updateFormBadge(); 
  document.querySelectorAll('.view').forEach(v=>v.classList.remove('active'));
  document.getElementById('view-dashboard').classList.add('active');
  document.querySelectorAll('.nav-tab').forEach((t,i)=>{ if(i===0) t.classList.add('active'); else t.classList.remove('active'); });
  STATE.activeView = 'dashboard';
}

function toggleAcc(trigger) {
  trigger.parentElement.classList.toggle('open');
}

function scrollToSection(id) {
  const el = document.getElementById('sec-'+id);
  if (el) {
    el.scrollIntoView({ behavior:'smooth', block:'start' });
    document.querySelectorAll('.toc-item').forEach(t=>t.classList.remove('active'));
    event.currentTarget.classList.add('active');
  }
}

// ─── INIT ─────────────────────────────────────────────────────────────────────
renderTxHistory();
renderMarkets();
simTick();
setInterval(simTick, 2500);

let autoPlayInterval = null;

function toggleAutoPlay() {
    const btn = document.getElementById('autoplay-btn');

    if (autoPlayInterval) {
        clearInterval(autoPlayInterval);
        autoPlayInterval = null;
        console.log("Simulator Paused.");
        btn.innerText = "▶ Auto-Play";
        btn.style.backgroundColor = "transparent"; 
        btn.style.color = ""; 
    } else {
        console.log("Simulator Running...");
        btn.innerText = "⏸ Pause Market";
        btn.style.backgroundColor = "var(--green)"; 
        btn.style.color = "#000";
        
        autoPlayInterval = setInterval(() => {
            nextTimeframe();
        }, 1000); 
    }
}