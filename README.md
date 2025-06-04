---

## 🛠️ 手順：自作バイナリを systemd デーモンとして起動する

---

### ① バイナリを配置する（例: `/usr/local/bin/keyboard_daemon`）

コンパイル済みのバイナリを適当な場所に置く：

```bash
sudo cp your_daemon_binary /usr/local/bin/keyboard_daemon
sudo chmod +x /usr/local/bin/keyboard_daemon
```

---

### ② systemd ユニットファイルを作成する

`/etc/systemd/system/keyboard_daemon.service` に以下のようなファイルを作る：

```ini
[Unit]
Description=My Custom Keyboard Daemon
After=multi-user.target

[Service]
ExecStart=/usr/local/bin/keyboard_daemon
Restart=on-failure
RestartSec=2
User=root
Group=root

[Install]
WantedBy=multi-user.target
```

📌 **ポイント**:

* `User=root` にしてるのは、キーボードデバイス（`/dev/input/eventX`）や `/dev/uinput` にアクセスするため。
* 必要なら `ExecStart` に引数も渡せるよ。

---

### ③ systemd に認識させて、有効化＆起動

```bash
sudo systemctl daemon-reexec
sudo systemctl daemon-reload
sudo systemctl enable keyboard_daemon.service
sudo systemctl start keyboard_daemon.service
```

---

### ④ 動作確認

```bash
sudo systemctl status keyboard_daemon.service
```

ログも確認できる：

```bash
journalctl -u keyboard_daemon.service
```

---

### 🧼 補足：ログアウト後も動かしたい場合

この設定でちゃんと常駐するから、ログインしてなくても問題なく動作するよ。まさにデーモン！

---

### 🧠 さらに自動化したいなら…

* **ログ出力をファイルにする** → `StandardOutput=file:/var/log/keyboard_daemon.log`
* **起動時のディレイを入れる**（たとえば他のデバイスが起動しきってから）
  → `ExecStartPre=/bin/sleep 2`

---

全部準備できたら `reboot` して、起動後に `keyboard_daemon` が自動で動作してるか確認してみて！

何か他に systemd 周りでやりたいことある？それとも `.service` ファイルを自動生成してみようか？
