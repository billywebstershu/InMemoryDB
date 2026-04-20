const API = 'http://127.0.0.1:9090';

// session stats
let totalQueries = 0;
let totalTime    = 0;
let fastestTime  = Infinity;
let chartLabels  = [];
let chartData    = [];

// authenticated flag
let authedUser = '';
let authedPass = '';

// chart setup
const ctx = document.getElementById('latencyChart').getContext('2d');
const chart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: chartLabels,
        datasets: [{
            label: 'Response Time (μs)',
            data: chartData,
            borderColor: 'rgb(34, 211, 238)',
            backgroundColor: 'rgba(34, 211, 238, 0.1)',
            borderWidth: 2,
            pointRadius: 3,
            tension: 0.3,
            fill: true
        }]
    },
    options: {
        responsive: true,
        animation: false,
        scales: {
            y: {
                beginAtZero: true,
                ticks: { color: '#9ca3af' },
                grid:  { color: '#374151' }
            },
            x: {
                ticks: { color: '#9ca3af' },
                grid:  { color: '#374151' }
            }
        },
        plugins: {
            legend: { labels: { color: '#9ca3af' } }
        }
    }
});

function authenticate() {
    authedUser = document.getElementById('authUser').value;
    authedPass = document.getElementById('authPass').value;

    if (!authedUser || !authedPass) {
        setAuthStatus('Please enter username and password', 'text-red-400');
        return;
    }

    // test connection with a KEYS request
    fetch(`${API}/api/keys`)
        .then(r => r.json())
        .then(() => {
            setAuthStatus(`Connected as ${authedUser}`, 'text-green-400');
        })
        .catch(() => {
            setAuthStatus('Could not connect to server', 'text-red-400');
        });
}

function setAuthStatus(msg, colour) {
    const el = document.getElementById('authStatus');
    el.textContent = msg;
    el.className = `py-2 ${colour}`;
}

function recordQuery(timeUs) {
    totalQueries++;
    totalTime += timeUs;
    if (timeUs < fastestTime) fastestTime = timeUs;

    // update stats
    document.getElementById('statTotal').textContent = totalQueries;
    document.getElementById('statAvg').textContent =
        (totalTime / totalQueries).toFixed(1) + ' μs';
    document.getElementById('statFastest').textContent =
        fastestTime + ' μs';

    // update chart
    chartLabels.push('#' + totalQueries);
    chartData.push(timeUs);

    // keep last 50 points on chart
    if (chartLabels.length > 50) {
        chartLabels.shift();
        chartData.shift();
    }

    chart.update();
}

function showResult(result, timeUs) {
    const resultEl = document.getElementById('resultValue');
    const timeEl   = document.getElementById('resultTime');

    resultEl.textContent = result;
    resultEl.className   = result === 'NULL' || result.startsWith('ERR')
        ? 'text-xl font-mono text-red-400'
        : 'text-xl font-mono text-green-400';

    timeEl.textContent = timeUs + ' μs';
    recordQuery(timeUs);
}

function sendGet() {
    const key = document.getElementById('keyInput').value;
    if (!key) { alert('Please enter a key'); return; }

    fetch(`${API}/api/get?key=${encodeURIComponent(key)}`)
        .then(r => r.json())
        .then(data => {
            showResult(data.result || 'NULL', data.response_time_us || 0);
        })
        .catch(() => showResult('ERR connection failed', 0));
}

function sendSet() {
    const key   = document.getElementById('keyInput').value;
    const value = document.getElementById('valueInput').value;
    if (!key || !value) { alert('Please enter a key and value'); return; }

    fetch(`${API}/api/set`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `key=${encodeURIComponent(key)}&value=${encodeURIComponent(value)}`
    })
        .then(r => r.json())
        .then(data => {
            showResult(data.result || 'ERR', data.response_time_us || 0);
        })
        .catch(() => showResult('ERR connection failed', 0));
}

function sendDel() {
    const key = document.getElementById('keyInput').value;
    if (!key) { alert('Please enter a key'); return; }

    fetch(`${API}/api/del?key=${encodeURIComponent(key)}`)
        .then(r => r.json())
        .then(data => {
            showResult(data.result || 'ERR', data.response_time_us || 0);
        })
        .catch(() => showResult('ERR connection failed', 0));
}

function sendKeys() {
    fetch(`${API}/api/keys`)
        .then(r => r.json())
        .then(data => {
            const keys = data.keys || [];
            const display = keys.length === 0
                ? 'EMPTY'
                : keys.join(', ');
            showResult(display, data.response_time_us || 0);
        })
        .catch(() => showResult('ERR connection failed', 0));
}

function runBulk() {
    const count = document.getElementById('bulkCount').value;

    fetch(`${API}/api/bulk`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `count=${count}`
    })
        .then(r => r.json())
        .then(data => {
            document.getElementById('bulkResults').classList.remove('hidden');
            document.getElementById('bulkCount2').textContent = data.count;
            document.getElementById('bulkAvg').textContent =
                data.avg_us + ' μs';
            document.getElementById('bulkMin').textContent =
                data.min_us + ' μs';
            document.getElementById('bulkMax').textContent =
                data.max_us + ' μs';

            recordQuery(data.avg_us);
        })
        .catch(() => alert('Bulk test failed — is the server running?'));
}