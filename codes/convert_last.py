import os
import tkinter as tk
from tkinter import filedialog, simpledialog
from PIL import Image
import struct
import shutil
import subprocess

# ================================================
# EKRANINA ÖZEL RENK DÖNÜŞÜMÜ (test ile kanıtlandı)
# ================================================
def rgb_to_bgr565(r, g, b):
    return ((b >> 3) << 11) | ((g >> 2) << 5) | (r >> 3)  # B5 G6 R5

def invert_for_screen(r, g, b):
    return (255 - r, 255 - g, 255 - b)  # ekranda doğru renk için tersle

# ================================================
# TEK GIF DÖNÜŞTÜR
# ================================================
def convert_gif_to_bin(gif_path):
    print(f"\nİşleniyor → {os.path.basename(gif_path)}")

    temp_dir = "temp_gif_frames"
    os.makedirs(temp_dir, exist_ok=True)

    # FFmpeg ile frame'leri çıkar (sessiz mod)
    subprocess.run([
        "ffmpeg", "-y", "-i", gif_path,
        "-vsync", "0",
        os.path.join(temp_dir, "frame_%04d.png")
    ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    png_files = sorted([f for f in os.listdir(temp_dir) if f.lower().endswith(".png")])
    if not png_files:
        print("   ❌ Frame çıkarılamadı! GIF bozuk olabilir.")
        shutil.rmtree(temp_dir, ignore_errors=True)
        return

    base_name = os.path.splitext(os.path.basename(gif_path))[0]
    out_bin = f"{base_name}_240x240.bin"   # artık _INV_B koymuyorum, çünkü bu "doğru" hali

    with open(out_bin, "wb") as f:
        for i, png in enumerate(png_files, 1):
            img = Image.open(os.path.join(temp_dir, png)).convert("RGB")
            img = img.resize((240, 240), Image.LANCZOS)   # en kaliteli resize

            for r, g, b in img.getdata():
                ri, gi, bi = invert_for_screen(r, g, b)      # tersle
                value = rgb_to_bgr565(ri, gi, bi)            # BGR565
                f.write(struct.pack("<H", value))

            print(f"   Frame {i:3d}/{len(png_files)} işlendi", end="\r")

    print(f"\n✅ OLUŞTURULDU → {out_bin} ({len(png_files)} frame)")
    shutil.rmtree(temp_dir)

# ================================================
# KLASÖR İÇİN HEPSİ
# ================================================
def convert_folder(folder_path):
    gifs = [f for f in os.listdir(folder_path) if f.lower().endswith((".gif", ".webp"))]
    if not gifs:
        print("Klasörde GIF/WEBP bulunamadı!")
        return

    print(f"\nToplam {len(gifs)} dosya bulundu:\n")
    for gif in gifs:
        full_path = os.path.join(folder_path, gif)
        convert_gif_to_bin(full_path)

# ================================================
# ANA MENÜ
# ================================================
def main():
    root = tk.Tk()
    root.withdraw()

    secim = simpledialog.askstring(
        "GIF → BIN Dönüştürücü",
        "1 = Tek dosya\n2 = Klasördeki tüm GIF'ler\n\nSeçiminiz (1/2): ",
        initialvalue="1"
    )

    if secim == "1":
        path = filedialog.askopenfilename(
            title="GIF seçin",
            filetypes=[("GIF ve WebP", "*.gif *.webp")]
        )
        if path:
            convert_gif_to_bin(path)

    elif secim == "2":
        folder = filedialog.askdirectory(title="GIF'lerin olduğu klasörü seçin")
        if folder:
            convert_folder(folder)

    else:
        print("Geçersiz seçim.")

    print("\nBitti! Çıkmak için Enter...")
    input()

if __name__ == "__main__":
    main()