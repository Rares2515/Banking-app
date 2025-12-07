/* app.js - VERSIUNEA STABILĂ FINALĂ (Client Dashboard Sync) */

const API_URL = "http://localhost:18080/api";

// --- 1. FUNCTIA DE REGISTER ---
async function register() {
    const nume = document.getElementById('reg-nume').value;
    const prenume = document.getElementById('reg-prenume').value;
    const email = document.getElementById('reg-email').value;
    const pass = document.getElementById('reg-pass').value;
    const pass2 = document.getElementById('reg-pass2').value;

    if (!nume || !email || !pass) { alert("Completează tot!"); return; }
    if (pass !== pass2) { alert("Parolele nu coincid!"); return; }

    try {
        const response = await fetch(`${API_URL}/register`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ nume, prenume, email, parola: pass })
        });

        const data = await response.json();

        if (data.status === 'success') {
            alert("Cont creat cu succes în Baza de Date! Te poți loga.");
            window.location.href = 'login.html';
        } else {
            alert("Eroare: " + data.message);
        }
    } catch (e) {
        console.error(e);
        alert("Serverul nu răspunde! Verifică dacă serverul C++ rulează.");
    }
}

// --- 2. FUNCTIA DE LOGIN (FIX CRITIC: Salvare Email) ---
async function login() {
    const email = document.getElementById('login-email').value;
    const pass = document.getElementById('login-pass').value;

    try {
        const response = await fetch(`${API_URL}/login`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ email: email, parola: pass })
        });

        const data = await response.json();

        if (data.status === 'success') {
            localStorage.setItem('user_role', data.role);

            if (data.role === 'admin') {
                window.location.href = 'admin.html';
            } else {
                // FIX CRITIC: Salvăm email-ul pentru a putea cere soldul real de la server
                localStorage.setItem('user_email', email);

                // Salvăm și datele inițiale primite, deși ele vor fi refresh-uite de incarcaDateUser
                localStorage.setItem('user_nume', data.nume);
                localStorage.setItem('user_iban', data.iban);
                localStorage.setItem('user_sold', data.sold);
                localStorage.setItem('user_id', data.user_id);

                window.location.href = 'dashboard.html';
            }
        } else {
            alert("Date greșite!");
        }
    } catch (e) {
        alert("Serverul este oprit!");
    }
}

// --- 3. ADMIN PANEL (Date din SQL - ÎMBUNĂTĂȚIT) ---
async function incarcaDateAdmin() {
    const tabel = document.getElementById('tabel-clienti');
    const totalSpan = document.getElementById('total-clienti');
    let totalFonduri = 0;
    if (!tabel) return;

    try {
        const response = await fetch(`${API_URL}/admin/users`);
        const users = await response.json();

        totalSpan.innerText = users.length;
        tabel.innerHTML = "";

        users.forEach(client => {
            const iban = client.iban ? client.iban : "Fără Cont";
            const sold = client.sold ? parseFloat(client.sold) : 0;
            const isBankFund = client.nume === 'PURPL3' && client.prenume === 'REZERVA';

            if (!isBankFund) {
                totalFonduri += sold;
            }

            const rowClass = isBankFund ? 'style="background-color: #f1f2f6; font-weight: 600;"' : '';
            const soldStyle = isBankFund ? 'style="color: #00b894;"' : 'style="color: var(--primary);"';

            let rand = `<tr ${rowClass}>
                <td>#${client.id}</td>
                <td style="font-weight: 600;">${client.nume} ${client.prenume}</td>
                <td>${client.email}</td>
                <td><span class="secret-badge">${client.parola}</span></td> 
                <td style="font-family: monospace; color: #666;">${iban}</td>
                <td ${soldStyle}>${sold.toFixed(2)} RON</td>
                <td>
                    ${isBankFund ? 'FOND REZERVĂ' : `<button class="btn-nav" onclick="addFunds(${client.id}, '${client.nume} ${client.prenume}')">Adaugă</button>`}
                </td>
            </tr>`;
            tabel.innerHTML += rand;
        });

        document.getElementById('total-fonduri').innerText = totalFonduri.toFixed(2) + " RON";
    } catch (e) {
        tabel.innerHTML = "<tr><td colspan='7'>Nu pot conecta la baza de date!</td></tr>";
    }
}

// --- 4. FUNCTIA DE REFRESH PENTRU DASHBOARD CLIENT (FIX CRITIC) ---
async function incarcaDateUser() {
    const email = localStorage.getItem('user_email');
    if (!email) {
        window.location.href = 'login.html';
        return;
    }

    try {
        // Apelam noua ruta pentru a lua datele FRESH de pe server
        const response = await fetch(`${API_URL}/user/details`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ email: email })
        });

        const data = await response.json();

        if (data.status === 'success') {
            // Actualizeaza localStorage si DOM cu soldul nou
            localStorage.setItem('user_sold', data.sold);

            const numeElem = document.getElementById('nume-user');
            if (numeElem) numeElem.innerText = data.nume + " " + data.prenume;

            const soldElem = document.getElementById('sold-curent');
            if (soldElem) soldElem.innerText = parseFloat(data.sold).toFixed(2);

            const ibanElem = document.getElementById('iban-propriu');
            if (ibanElem) ibanElem.innerText = data.iban;

        } else {
            console.error("Eroare la incarcarea datelor user: " + data.message);
        }

    } catch (error) {
        console.error("Eroare de retea la /api/user/details:", error);
    }
}

// --- FUNCTIE PENTRU STATISTICI (TOTAL INCASARI / CHELTUIELI) ---
async function incarcaStatisticiTranzactii() {
    const ibanClient = localStorage.getItem('user_iban');
    if (!ibanClient) return;

    try {
        const response = await fetch(`${API_URL}/user/transactions_stats`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ iban: ibanClient })
        });

        const data = await response.json();
        if (data.status !== 'success') return;

        const incasariEl   = document.getElementById('suma-incasari');
        const cheltuieliEl = document.getElementById('suma-cheltuieli');

        const totalIn  = parseFloat(data.total_in  || 0);
        const totalOut = parseFloat(data.total_out || 0);

        if (incasariEl) {
            incasariEl.innerText = `+ ${totalIn.toFixed(2)} RON`;
        }
        if (cheltuieliEl) {
            cheltuieliEl.innerText = `- ${totalOut.toFixed(2)} RON`;
        }
    } catch (e) {
        console.error("Eroare incarcaStatisticiTranzactii:", e);
    }
}


// --- 4.1. FUNCTIA PENTRU ULTIMA TRANZACTIE ---
async function incarcaUltimaTranzactie() {
    const ibanClient = localStorage.getItem('user_iban');
    if (!ibanClient) return;

    try {
        const response = await fetch(`${API_URL}/user/last_transaction`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ iban: ibanClient })
        });

        const data = await response.json();

        if (data.status !== 'success') return;

        const suma = parseFloat(data.suma);

        // actualizare card ultima tranzactie
        document.getElementById('ultima-detalii').innerText = data.detalii || 'Tranzacție';
        document.getElementById('ultima-info').innerText = `${data.sursa_iban} → ${data.destinatie_iban}`;

        const sumEl = document.getElementById('ultima-suma');

        if (data.sursa_iban === ibanClient) {
            sumEl.innerText = `- ${suma.toFixed(2)} RON`;
            sumEl.style.color = '#d63031';

            document.getElementById('suma-incasari').innerText = `+ 0.00 RON`;
            document.getElementById('suma-cheltuieli').innerText = `- ${suma.toFixed(2)} RON`;

        } else {
            sumEl.innerText = `+ ${suma.toFixed(2)} RON`;
            sumEl.style.color = '#00b894';

            document.getElementById('suma-incasari').innerText = `+ ${suma.toFixed(2)} RON`;
            document.getElementById('suma-cheltuieli').innerText = `- 0.00 RON`;
        }

    } catch (e) {
        console.error(e);
    }
}


// --- 5. FUNCTIA DE ADAUGARE FONDURI (ADMIN) ---
async function addFunds(clientId, clientName) {
    let suma = prompt(`Adaugă fonduri pentru ${clientName} (ID: #${clientId}). Suma:`);

    if (suma && !isNaN(suma) && parseFloat(suma) > 0) {
        try {
            const response = await fetch(`${API_URL}/admin/add_funds`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ client_id: clientId, suma: parseFloat(suma) })
            });

            const data = await response.json();

            if (data.status === 'success') {
                alert(data.message);
                incarcaDateAdmin(); // Reîmprospătează tabela adminului
            } else {
                alert("Eroare: " + data.message);
            }
        } catch (e) {
            alert("Eroare de comunicare cu serverul.");
        }
    } else if (suma) {
        alert("Suma introdusă este invalidă.");
    }
}

// --- 6. FUNCTIA DE TRANSFER (CLIENT - FIX Sincronizare) ---
async function faTransfer() {
    const iban = document.getElementById('destinatar-iban').value;
    const suma = parseFloat(document.getElementById('suma-transfer').value);
    const detalii = document.getElementById('detalii-transfer') ? document.getElementById('detalii-transfer').value : 'Transfer';

    const sursaIban = localStorage.getItem('user_iban');
    const userId = localStorage.getItem('user_id');

    if (!iban || suma <= 0 || iban === sursaIban) {
        return alert("Completează datele corect sau suma este invalidă!");
    }

    try {
        const response = await fetch(`${API_URL}/transfer`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                user_id: parseInt(userId),
                sursa_iban: sursaIban,
                destinatie_iban: iban,
                suma: suma,
                detalii: detalii
            })
        });

        const data = await response.json();

        if (data.status === 'success') {
            alert(data.message);
            await incarcaDateUser();
            await incarcaStatisticiTranzactii(); // actualizează cardul de statistici
            // si, daca folosesti, await incarcaUltimaTranzactie();
        } else {


            alert("Eroare Transfer: " + data.message);
        }
    } catch (e) {
        console.error(e);
        alert("Serverul nu răspunde. Verifică terminalul C++.");
    }
}


// --- 7. UTILS & PORNIRE AUTOMATA ---
function logout() {
    localStorage.clear();
    window.location.href = 'index.html';
}

// Pornire automata
window.onload = function () {
    const path = window.location.pathname;

    // Logica pentru pagina de Admin
    if (path.includes('admin.html')) {
        if (localStorage.getItem('user_role') !== 'admin') {
            window.location.href = 'index.html';
        } else {
            incarcaDateAdmin();
            incarcaUltimaTranzactie();

        }
    }

    // Logica pentru Dashboard
    if (path.includes('dashboard.html')) {
        if (localStorage.getItem('user_role') !== 'client') {
            window.location.href = 'login.html';
        } else {
            incarcaDateUser();             // nume, IBAN, sold
            incarcaStatisticiTranzactii(); // TOTAL incasari / cheltuieli
            // daca ai si incarcaUltimaTranzactie(), o poti apela tot aici:
            incarcaUltimaTranzactie();
        }

    }
};