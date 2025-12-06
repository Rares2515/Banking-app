/* app.js - LOGICA COMPLETA CU REGISTER FUNCTIONAL */

// --- 1. INITIALIZARE BAZA DE DATE (Simulata) ---
// Verificam daca avem deja useri salvati in browser. Daca nu, punem pe cei default.
if (!localStorage.getItem('db_users')) {
    const useriDefault = [
        {id: 1, nume: "Popescu", prenume: "Ion", email: "ion@test.com", parola: "123456", iban: "RO99BANC1000...", sold: 5000.00},
        {id: 2, nume: "Ionescu", prenume: "Maria", email: "maria@test.com", parola: "test", iban: "RO99BANC2000...", sold: 12500.50}
    ];
    localStorage.setItem('db_users', JSON.stringify(useriDefault));
}

// Functie ajutatoare sa citim userii oricand
function getUsers() {
    return JSON.parse(localStorage.getItem('db_users'));
}

// --- 2. FUNCTIA DE REGISTER ---
function register() {
    const nume = document.getElementById('reg-nume').value;
    const prenume = document.getElementById('reg-prenume').value;
    const email = document.getElementById('reg-email').value;
    const pass = document.getElementById('reg-pass').value;
    const pass2 = document.getElementById('reg-pass2').value;

    // Validari simple
    if(!nume || !prenume || !email || !pass) {
        alert("Te rugam sa completezi toate campurile!");
        return;
    }
    if(pass !== pass2) {
        alert("Parolele nu coincid!");
        return;
    }

    // Luam lista curenta de useri
    let users = getUsers();

    // Verificam daca emailul exista deja
    if(users.find(u => u.email === email)) {
        alert("Acest email este deja folosit!");
        return;
    }

    // Generam un IBAN random si un ID
    const newId = users.length + 1;
    const randomIbanSuffix = Math.floor(Math.random() * 1000000000000);
    const newIban = "RO99BANC" + randomIbanSuffix;

    // Cream noul user
    const newUser = {
        id: newId,
        nume: nume,
        prenume: prenume,
        email: email,
        parola: pass,
        iban: newIban,
        sold: 0.00 // Sold initial 0
    };

    // Il adaugam in lista si salvam
    users.push(newUser);
    localStorage.setItem('db_users', JSON.stringify(users));

    alert("Cont creat cu succes! Te poti loga acum.");
    window.location.href = 'login.html';
}

// --- 3. FUNCTIA DE LOGIN ---
function login() {
    const emailEl = document.getElementById('login-email');
    const passEl = document.getElementById('login-pass');
    
    if(!emailEl) return; // Suntem pe alta pagina
    
    const email = emailEl.value;
    const pass = passEl.value;

    // Admin Check
    if (email === 'admin' && pass === 'admin123') {
        localStorage.setItem('user_role', 'admin');
        window.location.href = 'admin.html';
        return;
    } 

    // Client Check (Citim din localStorage acum!)
    const users = getUsers();
    const userGasit = users.find(u => u.email === email && u.parola === pass);

    if (userGasit) {
        localStorage.setItem('user_role', 'client');
        // Salvam datele pentru dashboard
        localStorage.setItem('user_nume', userGasit.nume + " " + userGasit.prenume);
        localStorage.setItem('user_sold', userGasit.sold);
        localStorage.setItem('user_iban', userGasit.iban);
        
        window.location.href = 'dashboard.html';
    } else {
        alert("Email sau parola gresita!");
    }
}

// --- 4. ADMIN PANEL ---
function incarcaDateAdmin() {
    const tabel = document.getElementById('tabel-clienti');
    const totalSpan = document.getElementById('total-clienti');
    
    if (!tabel) return; // Nu suntem pe admin page

    // Citim userii reali din memorie
    const users = getUsers();

    totalSpan.innerText = users.length;
    tabel.innerHTML = "";

    users.forEach(client => {
        let rand = `<tr>
            <td>#${client.id}</td> <td style="font-weight: 600;">${client.nume} ${client.prenume}</td>
            <td>${client.email}</td>
            <td><span class="secret-badge">${client.parola}</span></td> <td style="font-family: monospace; color: #666;">${client.iban}</td>
            <td style="color: var(--primary); font-weight: bold;">${client.sold} RON</td>
        </tr>`;
        tabel.innerHTML += rand;
    });
}

// --- 5. UTILS (Logout, Dashboard) ---
function logout() {
    localStorage.removeItem('user_role');
    localStorage.removeItem('user_nume');
    window.location.href = 'index.html';
}

function incarcaDateUtilizator() {
    const elNume = document.getElementById('nume-user');
    const elSold = document.getElementById('sold-curent');
    const elIban = document.getElementById('iban-propriu');

    if(elNume) elNume.innerText = localStorage.getItem('user_nume');
    if(elSold) elSold.innerText = parseFloat(localStorage.getItem('user_sold')).toFixed(2);
    if(elIban) elIban.innerText = localStorage.getItem('user_iban');
}

// Verificari de securitate la incarcare
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

function faTransfer() {
    const iban = document.getElementById('destinatar-iban').value;
    const suma = document.getElementById('suma-transfer').value;
    const btn = document.querySelector('button.btn-login'); // Luam butonul

    if(!iban || !suma) {
        alert("Completeaza toate campurile!");
        return;
    }

    if(confirm(`Esti sigur ca vrei sa trimiti ${suma} RON catre ${iban}?`)) {
        
        // Efect vizual de incarcare
        const textInitial = btn.innerText;
        btn.innerText = "Se procesează...";
        btn.style.opacity = "0.7";

        setTimeout(() => {
            // Aici ar veni raspunsul de la server
            
            // Simulam actualizarea
            let soldCurent = parseFloat(localStorage.getItem('user_sold'));
            let sumaFloat = parseFloat(suma);

            if(soldCurent < sumaFloat) {
                alert("Fonduri insuficiente!");
            } else {
                let soldNou = soldCurent - sumaFloat;
                localStorage.setItem('user_sold', soldNou);
                
                // Actualizam ecranul
                document.getElementById('sold-curent').innerText = soldNou.toFixed(2);
                alert("Transfer realizat cu succes!");
                
                // Golim campurile
                document.getElementById('destinatar-iban').value = '';
                document.getElementById('suma-transfer').value = '';
            }

            // Revenim la butonul normal
            btn.innerText = textInitial;
            btn.style.opacity = "1";

        }, 1500); // Asteptam 1.5 secunde sa para ca "gandeste" serverul
    }
}