/* app.js - CONECTAT LA SERVER SQL */

const API_URL = "http://localhost:18080/api";

// --- 1. FUNCTIA DE REGISTER ---
async function register() {
    const nume = document.getElementById('reg-nume').value;
    const prenume = document.getElementById('reg-prenume').value;
    const email = document.getElementById('reg-email').value;
    const pass = document.getElementById('reg-pass').value;
    const pass2 = document.getElementById('reg-pass2').value;

    if(!nume || !email || !pass) { alert("Completează tot!"); return; }
    if(pass !== pass2) { alert("Parolele nu coincid!"); return; }

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
        alert("Serverul nu răspunde! Verifică dacă colegul a pornit serverul.");
    }
}

// --- 2. FUNCTIA DE LOGIN ---
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
            // Salvam in browser cine s-a logat
            localStorage.setItem('user_role', data.role);
            
            if (data.role === 'admin') {
                window.location.href = 'admin.html';
            } else {
                // E client, salvam datele primite de la SQL
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

// --- 3. ADMIN PANEL (Date din SQL) ---
async function incarcaDateAdmin() {
    const tabel = document.getElementById('tabel-clienti');
    const totalSpan = document.getElementById('total-clienti');
    if (!tabel) return;

    try {
        const response = await fetch(`${API_URL}/admin/users`);
        const users = await response.json();

        totalSpan.innerText = users.length;
        tabel.innerHTML = "";

        users.forEach(client => {
            // Tratare cazuri unde iban/sold sunt null (daca exista)
            const iban = client.iban ? client.iban : "Fără Cont";
            const sold = client.sold ? client.sold : 0;

            let rand = `<tr>
                <td>#${client.id}</td>
                <td style="font-weight: 600;">${client.nume} ${client.prenume}</td>
                <td>${client.email}</td>
                <td><span class="secret-badge">${client.parola}</span></td>
                <td style="font-family: monospace; color: #666;">${iban}</td>
                <td style="color: var(--primary); font-weight: bold;">${sold} RON</td>
            </tr>`;
            tabel.innerHTML += rand;
        });
    } catch (e) {
        tabel.innerHTML = "<tr><td colspan='6'>Nu pot conecta la baza de date!</td></tr>";
    }
}

// --- 4. DASHBOARD & UTILS ---
function logout() {
    localStorage.clear();
    window.location.href = 'index.html';
}

function incarcaDateUtilizator() {
    document.getElementById('nume-user').innerText = localStorage.getItem('user_nume');
    document.getElementById('sold-curent').innerText = parseFloat(localStorage.getItem('user_sold')).toFixed(2);
    document.getElementById('iban-propriu').innerText = localStorage.getItem('user_iban');
}

// Functia Transfer ramane vizuala momentan (sau o putem lega la SQL ulterior)
function faTransfer() {
    const iban = document.getElementById('destinatar-iban').value;
    const suma = document.getElementById('suma-transfer').value;
    
    if(!iban || !suma) return alert("Completează datele!");

    // Simulam doar vizual scaderea pana cand facem ruta de transfer in C++
    let sold = parseFloat(localStorage.getItem('user_sold'));
    let dePlata = parseFloat(suma);

    if(sold >= dePlata) {
        sold -= dePlata;
        localStorage.setItem('user_sold', sold);
        document.getElementById('sold-curent').innerText = sold.toFixed(2);
        alert("Transfer trimis! (Actualizarea în SQL necesită implementare extra)");
    } else {
        alert("Fonduri insuficiente!");
    }
}

// Pornire automata
window.onload = function() {
    const path = window.location.pathname;
    if(path.includes('admin.html')) {
        if(localStorage.getItem('user_role') !== 'admin') window.location.href = 'index.html';
        incarcaDateAdmin();
    }
    if(path.includes('dashboard.html')) {
        if(localStorage.getItem('user_role') !== 'client') window.location.href = 'login.html';
        incarcaDateUtilizator();
    }
};
