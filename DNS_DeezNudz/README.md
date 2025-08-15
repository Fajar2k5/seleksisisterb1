# Simulasi Jaringan dengan DNS, Reverse Proxy, dan Firewall

Link video: https://youtu.be/-6sv6-hKxn4

## Arsitektur

Semua VM berjalan pada jaringan virtual terisolasi (`Internal Network` di VirtualBox) dan tidak terhubung ke internet. Gunakan VM Debian agar bisa mereproduksi konfigurasi gwejh :)

* **VM1 (DNS & DHCP Server)**: `192.168.56.10`
* **VM2 (HTTP Server)**: `192.168.56.20`
* **VM4 (Reverse Proxy & Firewall)**: `192.168.56.30`
* **VM3 (Client)**: Dinamis (DHCP) kecuali diatur manual

**Alur:**
`Klien (VM3) -> DNS (VM1) -> Reverse Proxy (VM4) -> HTTP Server (VM2)`

---

## Config per VM

### VM 1: DNS & DHCP Server

#### Requirements
```bash
sudo apt-get install bind9 isc-dhcp-server
```

#### Konfigurasi
* `/etc/bind/named.conf.options`: Konfigurasi global BIND9. Diatur agar hanya mendengarkan (`listen-on`) dan menjawab query (`allow-query`) dari jaringan internal simulasi.
* `/etc/bind/named.conf.local`: Mendaftarkan domain custom (`mysite.local`).
* `/etc/bind/zones/db.mysite.local`: File database DNS. `www` diarahkan ke IP Reverse Proxy (`192.168.56.30`).
* `/etc/dhcp/dhcpd.conf`: Secara otomatis memberikan alamat IP ke klien (`192.168.56.10`).

#### Langkah Singkat
1.  Edit /etc/network/interfaces dan sesuaikan untuk menambahkan isi dari file interfaces pada folder DNS_DeezNudz ini, sesuaikan jika nama interface berbeda, lalu `sudo systemctl restart networking.service`
2.  Install `bind9` dan `isc-dhcp-server`.
3.  Salin file-file konfigurasi ke lokasi yang sesuai.
4.  Restart layanan: `sudo systemctl restart bind9 isc-dhcp-server`.

### VM 2: HTTP Server (Backend)

#### Requirements
```bash
sudo apt-get install nginx
```

#### Konfigurasi
* `/etc/nginx/sites-available/mysite`: Konfigurasi Nginx untuk menjalankan web server pada port non-standar (`listen 8080;`).
* `/var/www/html/mysite/index.html`: File `index.html` untuk website.

#### Langkah Singkat
1.  Edit /etc/network/interfaces dan sesuaikan untuk menambahkan isi dari file interfaces pada folder DNS_DeezNudz ini, sesuaikan jika nama interface berbeda, kali ini ganti address menjadi 192.168.56.20, lalu `sudo systemctl restart networking.service`
2.  Install `nginx`.
3.  Buat direktori web dan file `index.html`.
4.  Salin file konfigurasi Nginx, lalu aktifkan dengan membuat symbolic link `ln -s /etc/nginx/sites-available/mysite /etc/nginx/sites-enabled/` lalu hapus default `rm /etc/nginx/sites-enabled/default`.
5.  Restart layanan: `sudo systemctl restart nginx`.

### VM 4: Reverse Proxy & Firewall

#### Perangkat Lunak yang Diinstal
```bash
sudo apt-get install nginx ufw
```

#### Konfigurasi
* `/etc/nginx/sites-available/reverse-proxy`: Konfigurasi Nginx sebagai Reverse Proxy. `proxy_pass http://192.168.56.20:8080;` meneruskan semua permintaan ke VM2.
* **Aturan Firewall (UFW)**: Dikonfigurasi melalui command.
    * Mengizinkan trafik masuk ke port `8080` (web) dan `22` (SSH).
    * Memblokir (`deny`) trafik dari rentang IP tertentu. Aturan `deny` harus disisipkan di urutan pertama (`insert 1`) agar dieksekusi sebelum aturan `allow`.

#### Langkah Singkat
1.  Edit /etc/network/interfaces dan sesuaikan untuk menambahkan isi dari file interfaces pada folder DNS_DeezNudz ini, sesuaikan jika nama interface berbeda, kali ini ganti address menjadi 192.168.56.30, lalu `sudo systemctl restart networking.service`
2.  Install `nginx` dan `ufw`.
3.  Salin file konfigurasi Nginx.
4.  Restart Nginx: `sudo systemctl restart nginx`.
5.  Nyalakan dengan `ln -s /etc/nginx/sites-available/reverse-proxy /etc/nginx/sites-enabled/` dan hapus default`rm /etc/nginx/sites-enabled/default`
6.  Atur firewall: `sudo ufw default deny incoming`, `sudo ufw default allow outgoing`, `sudo allow ssh`, `sudo allow 8080/tcp`, `sudo ufw insert 1 deny from 192.168.56.144/28 to any`, `sudo ufw enable`.

### VM 3: Client

#### Requirement
```bash
sudo apt-get install python3 curl network-manager
```

#### File & Skrip
* `client.py`:
    * Mengalihkan mode jaringan antara DHCP dan IP Statis dengan `nmcli`.
    * Memvalidasi input.
    * Menampilkan IP yang baru diset.
    * Menguji koneksi ke website dengan `curl`.
* `/etc/network/interfaces`: File ini sengaja dikosongkan (hanya menyisakan `lo`) untuk menyerahkan kontrol penuh jaringan kepada `NetworkManager`, sehingga skrip `client.py` dapat berfungsi dengan baik. Jadi kalau VM yang lain ditambahkan isi dari file interfaces di repo ini, untuk VM ini jangan, malah harus dihapus.

#### Langkah Singkat
1.  Install `python3`, `curl`, dan `network-manager`.
2.  Pastikan file `/etc/network/interfaces` bersih dari konfigurasi interface.
3.  Salin skrip `client.py` ke VM.
4.  Jalankan simulasi.

---

## Cara Menjalankan Simulasi

1.  Nyalakan semua VM (VM1, VM2, VM4, dan VM3).
2.  Masuk ke terminal **VM3**.
3.  Jalankan skrip klien dengan hak akses root:
    ```bash
    sudo python3 client.py
    ```
4.  **Uji Akses Normal**:
    * Pilih menu **1 (DHCP)**. Script akan mengonfigurasi jaringan dan menampilkan IP yang didapat.
    * Pilih menu **3 (Akses Website)**. Melihat output dari `curl` yang menunjukkan koneksi berhasil.
5.  **Uji Firewall**:
    * Pilih menu **2 (Manual)**. Menginput alamat IP yang berada dalam rentang yang diblokir (misal: `192.168.56.151`).
    * Pilih menu **3 (Akses Website)**. Koneksi sekarang seharusnya gagal (timeout), membuktikan firewall bekerja.
