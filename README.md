# Plant Watering Machine Learning (Kebun Pintar)

Proyek UTS berupa sistem penyiraman tanaman otomatis. Data dari sensor dipakai untuk melatih
model Machine Learning yang menentukan apakah tanaman perlu disiram atau belum.

- **Mata kuliah:**  Workshop Pembelajaran Mesin 
- **Kelompok / NIM:** ADRIYAN RAHMAD RAMADHAN / 230935605370 (Anggota)
                      CESAR SUGENG DARMAWAN / 230935608959 (Ketua) 
                      KARINA PUSPITA SEMBADA / 230935608745 (Anggota) 
- **Dosen:** Andarta Fardhanul Khoir, S.Kom., M.Comp.

## 1. Deskripsi Singkat

Sensor membaca kelembapan tanah (mV) dan suhu lingkungan (°C). Dari data itu, model
mengelompokkan kondisi tanaman ke dua kelas, yaitu `BELUM_PERLU` dan `PERLU_DISIRAM`.
Hasil prediksi nantinya dipakai untuk menyalakan pompa air.

## 2. Draft Alur Sistem

```
Sensor kelembapan tanah dan sensor suhu
        ↓
Mikrokontroler membaca data (arduino/*.ino)
        ↓
Data disimpan ke data/dataset_kebun_pintar.csv
        ↓
Preprocessing: cleaning, encoding label kondisi tanah, scaling
        ↓
Training model: Random Forest dan Decision Tree (SVM RBF sebagai pembanding)
        ↓
Evaluasi pakai akurasi dan confusion matrix, lalu pilih model baseline
        ↓
Prediksi: PERLU_DISIRAM atau BELUM_PERLU
        ↓
Jika PERLU_DISIRAM, pompa air dinyalakan lewat relay
```

## 3. Struktur Repository

| Folder / File | Keterangan |
|---|---|
| `data/dataset_kebun_pintar.csv` | Dataset hasil pembacaan sensor |
| `notebooks/00_All_Output_Code.ipynb` | Seluruh pipeline dari awal sampai akhir, lengkap dengan output |
| `notebooks/01_Random_Forest.ipynb` | Training dan evaluasi Random Forest |
| `notebooks/02_Decision_Tree.ipynb` | Training dan evaluasi Decision Tree |
| `notebooks/03_SVM_RBF.ipynb` | Percobaan SVM (RBF) |
| `models/model_rf.pkl`, `model_dt.pkl` | Model hasil training |
| `models/scaler_rf.pkl`, `scaler_dt.pkl` | Scaler untuk normalisasi data |
| `models/encoder_rf.pkl`, `encoder_dt.pkl` | Encoder untuk label |
| `arduino/Plant_watering_machine_learning/` | Kode Arduino untuk membaca sensor dan mengontrol pompa |
| `results/` | Grafik dan hasil output |

## 4. Akuisisi Data dan Preprocessing

- **Sumber data:** pembacaan sensor langsung, total 265 data.
- **Fitur:** kelembapan tanah (mV), suhu lingkungan (°C), dan kondisi tanah yang sudah di-encode (`Kondisi_enc`).
- **Kondisi tanah:** `kering` 111 data, `cukup` 69 data, `basah` 85 data.
- **Target:** `pump_target` (1 = perlu disiram, 0 = belum perlu).
- **Cleaning:** [isi cara menangani missing values / noise]
- **Scaling:** [StandardScaler / MinMaxScaler], disimpan di `models/scaler_*.pkl`.
- **Pembagian data:** 212 data latih dan 53 data uji (80:20).

### Eksplorasi Data

![Distribusi Kondisi Tanah dan Hubungan Kelembapan Tanah dengan Suhu](results/Grafik%20Distribusi%20Kondisi%20Tanah%20dan%20Grafik%20Hubungan%20Kelembapan%20Tanah%20dengan%20Suhu%20Lingkungan.png)

Pada grafik di atas, semua data dengan kondisi `kering` punya `pump_target = 1`, sedangkan
kondisi `cukup` dan `basah` punya `pump_target = 0`.

![Perbandingan Akurasi dan Sebaran Kelembapan Tanah per Kondisi](results/Perbandingan%20Akurasi%20Model%20Random%20Forest%20dan%20Decision%20Tree%20dan%20Grafik%20Distribusi%20Kelembapan%20Tanah%20Berdasarkan%20Kondisi%20Tanah.png)

Nilai sensor kelembapan tanah terlihat berbeda untuk tiap kondisi. Tanah kering ada di
sekitar 2000 sampai 3150 mV, tanah cukup sekitar 150 sampai 200 mV, dan tanah basah sekitar 50 mV.

## 5. Model Baseline

Ada dua model yang dibandingkan, diuji pada 53 data uji:

| Model | Akurasi |
|---|---|
| Random Forest | 100.00% |
| Decision Tree | 100.00% |

![Hasil Perbandingan Model](results/Hasil%20test%20training.png)

### Confusion Matrix

![Confusion Matrix Random Forest dan Decision Tree](results/Confusion%20Matrix%20Model%20Random%20Forest%20dan%20Confusion%20Matrix%20Model%20Decision%20Tree.png)

Kedua model menebak semua data uji dengan benar: 31 data `BELUM_PERLU` dan 22 data
`PERLU_DISIRAM`, tanpa ada yang salah.

### Struktur Decision Tree

![Struktur Decision Tree](results/Structure%20decision.png)

Pohonnya hanya punya kedalaman 2. Pemisahan pertama memakai `Kondisi_enc <= 0.465`.
Cabang kanan (89 sampel) langsung masuk `PERLU_DISIRAM`, sedangkan cabang kiri (123 sampel)
masuk `BELUM_PERLU`.

**Alasan memilih algoritma:** Random Forest dan Decision Tree cocok untuk data sensor yang
berbentuk tabel, tidak butuh data yang sangat banyak, dan tidak terpengaruh skala fitur.
Decision Tree juga mudah dilihat dan dijelaskan lewat gambar pohonnya.

## 6. Catatan

Akurasi kedua model 100%, tetapi perlu dibaca hati-hati. Decision Tree hanya memakai
fitur `Kondisi_enc`, sementara `pump_target` memang ditentukan langsung dari kondisi tanah.
Jadi data ini mudah dipisahkan, dan jumlah data ujinya juga baru 53.

## 7. Cara Menjalankan

1. Clone repo: `git clone (https://github.com/adriyanramadhan-maker/plant-watering-ml.git)`
2. Install library: `pip install pandas numpy scikit-learn joblib jupyter matplotlib seaborn`
3. Buka Jupyter dengan `jupyter notebook`
4. Jalankan `notebooks/00_All_Output_Code.ipynb` dari atas sampai bawah.
