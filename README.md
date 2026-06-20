<div align="center">

# ⚡ Tradealyze

### *A high-performance C++ matching engine, wired live to a real-time web trading terminal*

![C++](https://img.shields.io/badge/C%2B%2B-14%2F17-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![WebSocket](https://img.shields.io/badge/WebSocket-Crow%20%2B%20Asio-blue?style=for-the-badge&logo=websocket&logoColor=white)
![Architecture](https://img.shields.io/badge/Architecture-Full--Stack-orange?style=for-the-badge)
![Status](https://img.shields.io/badge/Status-Complete-success?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-lightgrey?style=for-the-badge)

</div>

---

## 📖 Overview

Tradealyze is a full-stack, real-time trading simulator that bridges a memory-safe, low-latency C++ matching engine with a responsive vanilla JavaScript web terminal. The C++ core ingests historical market data, executes genuine price-time priority order matching, and broadcasts live order book and wallet state to the browser over persistent WebSockets — no polling, no stale data.

The project started as an OOP capstone (University of London, Coursera) and was rebuilt into a complete full-stack system: a multithreaded Crow + Asio server handles bidirectional JSON messaging, the `OrderBook` engine performs O(n log n) initial sorting followed by O(n) incremental insertion via `std::lower_bound`, and every fill triggers atomic, server-authoritative wallet updates across multiple currencies. The frontend is a zero-build, zero-dependency SPA dashboard rendering live depth charts, transaction history, and dynamic market pairs.

> Most trading simulators are either slow browser-only mocks with no real matching logic, or C++ engines locked away with no visual interface. Tradealyze closes that gap — a deterministic, testable matching core that *looks and behaves* like a real trading terminal.

---

## ✨ Key Highlights

| | Highlight |
|---|---|
| ⚡ | Sub-10ms state propagation via persistent WebSockets — no REST polling |
| 📊 | Genuine price-time priority matching engine, not a frontend mock |
| 💰 | Server-authoritative, atomic multi-currency wallet tracking (BTC, ETH, USDT, AVAX) |
| 🧵 | Multithreaded Crow server handling 1000+ concurrent connections |
| 📈 | 3,500+ historical ETH/BTC orders ingested from CSV with malformed-row tolerance |
| 🔌 | Product-agnostic design — swap CSVs to simulate forex, equities, or other crypto pairs |

---

## 🧠 Core Technical Concept: Matching Engine & Data Flow

| Component | File(s) | Use Case | Key Detail |
|---|---|---|---|
| Order Book | `OrderBook.h/.cpp` | Price-time priority matching per product | Asks sorted ascending, bids descending, FIFO tiebreak via monotonic `orderId` |
| Order Entry | `OrderBookEntry.h/.cpp` | Order struct + comparators | Pointer-based mutation on the master vector — zero copy overhead |
| Wallet | `Wallet.h/.cpp` | Multi-currency balance tracking | Spend validation before insert; atomic credit/debit on fill |
| CSV Reader | `CSVReader.h/.cpp` | Historical data ingestion | O(n log n) sort, then O(n) insertion via `std::lower_bound`; skips malformed rows with logged line numbers |
| Server | `Trading.cpp` | Crow WebSocket/HTTP router | Message-type-based routing, decoupled from product logic |
| Console Fallback | `MerkelMain.cpp` | Optional CLI simulation interface | Standalone testing without the web frontend |

<details>
<summary><strong>📄 Show implementation snippets</strong></summary>

```cpp
// Price-time priority: ascending asks, descending bids, FIFO via monotonic orderId
std::sort(asks.begin(), asks.end(), [](const OrderBookEntry& a, const OrderBookEntry& b) {
    return a.price < b.price; // cheapest ask first
});
```

```cpp
// Atomic wallet update on a confirmed fill — buy credits base, debits quote
void Wallet::processSale(const OrderBookEntry& sale) {
    std::string base = sale.product.substr(0, sale.product.find('/'));
    std::string quote = sale.product.substr(sale.product.find('/') + 1);
    // buy -> credit base / debit quote, sell -> debit base / credit quote
}
```

| Operation | Complexity | Notes |
|---|---|---|
| Initial CSV sort | O(n log n) | One-time, on load |
| Incremental insert | O(n) | Via `std::lower_bound` per new order |
| Order matching pass | O(n) per timeframe | Linear scan against sorted book |

</details>

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                  Tradealyze C++ Core                         │
│  ├─ OrderBook    → Price-time priority matching              │
│  ├─ Wallet       → Multi-currency balance tracking           │
│  ├─ CSVReader    → Historical data ingestion                 │
│  ├─ Crow         → HTTP/WebSocket routing                    │
│  └─ Asio         → Async I/O & multithreading                │
└─────────────────────────────────────────────────────────────┘
                          │
                          ▼  WebSocket (ws://localhost:9002)
┌─────────────────────────────────────────────────────────────┐
│                Vanilla JS SPA Dashboard                      │
│  ├─ Real-time Order Book   (sticky-header, scrollable)       │
│  ├─ Transaction History    (independent scroll panel)        │
│  ├─ Dynamic Market Pairs   (sortable, filterable grid)       │
│  └─ Wallet Balances        (live-updating header)            │
└─────────────────────────────────────────────────────────────┘
```

```
CSV Data → CSVReader → OrderBook (match) → Wallet (settle) → Crow (broadcast) → JS Dashboard
                ▲                                                                    │
                └──────────────────────── submit_order / next_timeframe ◄────────────┘
```

---

## 🧩 Design / OOP Architecture

- **`OrderBook`** — owns the master order vector per product; performs matching, partial fills, and depth aggregation.
- **`OrderBookEntry`** — represents a single order (side, price, amount, type, timestamp, orderId) with comparator logic for sorting.
- **`Wallet`** — tracks balances per currency, validates spend capacity before order acceptance, applies atomic updates on fill.
- **`CSVReader`** — parses `RecordBook.csv`, tolerates malformed rows, and feeds the initial order book state.
- **`Trading.cpp` (Crow server)** — routes incoming WebSocket messages by `action` type (`submit_order`, `next_timeframe`) and broadcasts `orderbook`, `trade`, `balances`, `error`, and `products` messages back to clients.

**Design Principle — Decoupling for Scale:** The engine is intentionally product-agnostic.
- `OrderBook` operates on any `BASE/QUOTE` pair parsed from CSV — swapping the CSV simulates forex, equities, or other crypto pairs instantly.
- The Crow router dispatches by message *type*, not hardcoded product logic, so the matching engine never needs to change to support new instruments.
- Networking (Crow) is isolated from matching logic — it could be replaced with Boost.Beast or uWebSockets without touching `OrderBook` or `Wallet`.
- An authentication layer could be added purely as Crow middleware, leaving the engine untouched.

---

## 📦 Core Modules / Features

| Module | Purpose | Key Features |
|---|---|---|
| Matching Engine | Execute trades against the order book | Price-time priority, partial fills, FIFO tiebreaking, zero-copy mutation |
| Wallet System | Track and validate balances | Multi-currency, spend validation, atomic credit/debit on every fill |
| Data Ingestion | Load historical market data | 3,500+ rows, malformed-row tolerance with line-number logging |
| Timeframe Engine | Drive simulation forward | 6 stages (Genesis → After Hours), manual or 1s auto-play, looping clock |
| Trading Terminal UI | Visualize live market state | Depth-bar order book, scroll-isolated transaction history, sortable/filterable pairs grid, live connection badge |

---

## 🗂️ Data Persistence

| File | Role | Notes |
|---|---|---|
| `RecordBook.csv` | Primary historical order feed | 3,500+ ETH/BTC orders |
| `RecordBook_Clean.csv` | Sanitized dataset | Pre-processed via `clean_csv.py` for ingestion reliability |

---

## 🛠️ Tech Stack

- **Language:** C++14/17
- **Networking:** Asio (standalone), multithreaded async I/O
- **Web Framework:** Crow — lightweight, header-only WebSocket/HTTP routing
- **Serialization:** nlohmann/json
- **Frontend:** HTML5, CSS3, vanilla JavaScript — no build step, no dependencies
- **Protocol:** WebSocket (JSON messages)
- **Data Source:** CSV-based historical market data

---

## 🚀 Getting Started

```bash
# Clone the repository
git clone https://github.com/Abdul-Hanan-07/Tradealyze.git
cd Tradealyze

# Compile (Linux/macOS)
g++ -std=c++17 Trading.cpp MerkelMain.cpp OrderBook.cpp OrderBookEntry.cpp \
    Wallet.cpp CSVReader.cpp -I asio_include -DASIO_STANDALONE -pthread -o tradealyze

# Compile (Windows / MinGW-w64)
g++ Trading.cpp MerkelMain.cpp OrderBook.cpp OrderBookEntry.cpp Wallet.cpp \
    CSVReader.cpp -I asio_include -DASIO_STANDALONE -pthread -lws2_32 -lwsock32 -o tradealyze

# Run the server
./tradealyze
# Server starts on ws://localhost:9002
```

Open `index.html` (in the `templete` folder) in any modern browser — no build step required.

> 💡 **First run tip:** Ensure `RecordBook.csv` is in the working directory at runtime; the matching engine seeds its initial order book from this file on startup.

---

## 🖥️ Project Output / Demo

```
[CONNECTED] ws://localhost:9002

  ETH/BTC Order Book
  ─────────────────────────────
   ASKS                 amount   total
   0.02190               3.20     8.20
   0.02189               5.00     5.00
   ─────────────────────────────
   0.02187               2.50     2.50
   0.02186               4.00     6.50
   BIDS

  Wallet → BTC: 9.78109  ETH: 10.0  USDT: 10000.0
```

```
> next_timeframe
[Genesis → Morning] 14 trades matched, order book rebroadcast
```

---

## 🔮 Future Enhancements

- [ ] Live price feed integration (e.g. Binance WebSocket API)
- [ ] User authentication with persistent order history
- [ ] Docker containerization + cloud deployment
- [ ] Order cancellation and modification endpoints
- [ ] WebGL candlestick charts replacing SVG sparklines
- [ ] Horizontal sharding — one `OrderBook` thread per product with lock-free queues

---

<div align="center">

## 👤 Author

**Abdul Hanan**
BSCS Student, University of the Punjab
Built as a full-stack evolution of a C++ OOP capstone project

</div>

---

<div align="center">

⭐ If you found this project interesting, consider giving it a star!

</div>
