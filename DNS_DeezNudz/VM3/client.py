import os
import sys
import subprocess
import ipaddress
import time
import json

# Ganti jika di device berbeda
INTERFACE_NAME = "enp0s3"

# IP DNS server (VM1)
DNS_SERVER = "192.168.56.10"

# Subnet yang diizinkan untuk konfigurasi IP manual.
ALLOWED_SUBNET = ipaddress.ip_network("192.168.56.0/24")

# Domain target
TARGET_DOMAIN = "www.mysite.local"

def clear_screen():
    os.system('clear')

def run_command(command, capture_output=False):
    try:
        result = subprocess.run(
            command,
            shell=True,
            check=True,
            text=True,
            capture_output=capture_output
        )
        if capture_output:
            return result.stdout.strip()
        return True
    except subprocess.CalledProcessError as e:
        print(f"\nError saat menjalankan perintah: '{e.cmd}'")
        # Pesan error dari stderr seringkali lebih informatif
        error_message = e.stderr.strip() if e.stderr else "Tidak ada output error."
        print(f"Pesan Error: {error_message}")
        return None
    except FileNotFoundError:
        print(f"\nError: Perintah '{command.split()[0]}' tidak ditemukan. Pastikan sudah terinstall.")
        return None

def get_connection_name():
    print(f"Mencari nama koneksi untuk interface '{INTERFACE_NAME}'...")
    cmd = f"nmcli -t -f NAME,DEVICE con show --active | grep ':{INTERFACE_NAME}$' | cut -d: -f1"
    connection_name = run_command(cmd, capture_output=True)
    if connection_name is not None and connection_name != '':
        print(f"Koneksi ditemukan: '{connection_name}'")
        return connection_name
    else:
        print(f"Tidak dapat menemukan nama koneksi aktif untuk '{INTERFACE_NAME}'.")
        print("Pastikan interface memiliki profil koneksi yang aktif.")
        sys.exit(1)

def is_valid_ip(ip_str):
    try:
        ip = ipaddress.ip_address(ip_str)
        if not ip.is_private:
            print("Error: Alamat IP harus merupakan alamat privat (e.g., 192.168.x.x).")
            return False
        if ip not in ALLOWED_SUBNET:
            print(f"Error: Alamat IP harus berada dalam subnet simulasi ({ALLOWED_SUBNET}).")
            return False
        if ip == ALLOWED_SUBNET.network_address or ip == ALLOWED_SUBNET.broadcast_address:
            print("Error: Tidak bisa menggunakan alamat network atau broadcast.")
            return False
        if str(ip) == DNS_SERVER:
            print("Error: Tidak bisa menggunakan alamat IP dari DNS Server.")
            return False
    except ValueError:
        print("Error: Format alamat IP tidak valid.")
        return False
    return True

def show_current_ip(interface_name):
    print(f"\nMengecek alamat IP untuk '{interface_name}'...")
    cmd = f"ip -4 -j addr show {interface_name}"
    json_output = run_command(cmd, capture_output=True)

    if json_output:
        try:
            data = json.loads(json_output)
            if data and data[0].get('addr_info'):
                ip_address = data[0]['addr_info'][0]['local']
                prefix = data[0]['addr_info'][0]['prefixlen']
                print(f"IP Address yang di-assign: {ip_address}/{prefix}")
                return
            else:
                print(f"Tidak ada alamat IPv4 yang terkonfigurasi di '{interface_name}'.")
        except (json.JSONDecodeError, IndexError, KeyError):
            print(f"Gagal mem-parsing output IP address untuk '{interface_name}'.")
    else:
        print(f"Gagal mendapatkan informasi IP untuk '{interface_name}'.")


def reactivate_connection(connection_name):
    print("Mengaktifkan ulang koneksi untuk menerapkan perubahan...")
    if run_command(f"sudo nmcli con down \"{connection_name}\"") and \
       run_command(f"sudo nmcli con up \"{connection_name}\""):
        print("Jaringan berhasil diaktifkan ulang.")
        time.sleep(3)  # tunggu jaringan stabil
        return True
    else:
        print("Gagal mengaktifkan ulang jaringan.")
        return False

def configure_dhcp(connection_name):
    print("\nMengonfigurasi jaringan ke mode DHCP...")

    cmd_reset_to_dhcp = f"sudo nmcli con mod \"{connection_name}\" ipv4.addresses '' ipv4.gateway '' ipv4.dns '' ipv4.method auto"

    if run_command(cmd_reset_to_dhcp):
        print("Konfigurasi DHCP berhasil diterapkan.")
        if reactivate_connection(connection_name):
            show_current_ip(INTERFACE_NAME)
    else:
        print("Gagal beralih ke mode DHCP.")

def configure_manual(connection_name):
    print("\nSilakan masukkan alamat IP statis untuk klien ini.")
    while True:
        ip_manual = input(f"Masukkan IP (contoh: 192.168.56.40): ")
        if is_valid_ip(ip_manual):
            break

    print("\nMengonfigurasi jaringan ke mode Manual...")
    gateway = str(ALLOWED_SUBNET.network_address + 1)

    cmd_set_manual = (
        f"sudo nmcli con mod \"{connection_name}\" "
        f"ipv4.method manual "
        f"ipv4.addresses {ip_manual}/{ALLOWED_SUBNET.prefixlen} "
        f"ipv4.gateway {gateway} "
        f"ipv4.dns \"{DNS_SERVER}\""
    )

    if run_command(cmd_set_manual):
        print("Konfigurasi manual berhasil diterapkan.")
        if reactivate_connection(connection_name):
            show_current_ip(INTERFACE_NAME)
    else:
        print("Gagal menerapkan konfigurasi manual.")

def access_website():
    port = "8080"
    url = f"http://{TARGET_DOMAIN}:{port}"

    print(f"\n🌐 Mencoba mengakses: {url}\n")
    print("-" * 40)
    run_command(f"curl -v --connect-timeout 5 {url}")
    print("-" * 40)


def main():
    if os.geteuid() != 0:
        print("Error: Skrip ini harus dijalankan dengan hak akses root.")
        print("use: sudo python3 client.py")
        sys.exit(1)

    clear_screen()
    print("===== CLI Client =====")
    connection_name = get_connection_name()

    while True:
        print("\n----------------- MENU -----------------")
        print(f"1. IP Otomatis (DHCP)")
        print(f"2. Konfigurasi IP Manual (IP Statis)")
        print(f"3. Akses Website ({TARGET_DOMAIN})")
        print(f"4. Tampilkan Konfigurasi IP Saat Ini")
        print(f"5. Keluar")

        choice = input("\nMasukkan pilihan Anda [1-5]: ")

        if choice == '1':
            configure_dhcp(connection_name)
        elif choice == '2':
            configure_manual(connection_name)
        elif choice == '3':
            access_website()
        elif choice == '4':
            show_current_ip(INTERFACE_NAME)
        elif choice == '5':
            print("\nTerima kasih! Keluar dari program.")
            break
        else:
            print("Pilihan tidak valid, silakan coba lagi.")

        input("\nTekan Enter untuk kembali ke menu...")
        clear_screen()

if __name__ == "__main__":
    main()
