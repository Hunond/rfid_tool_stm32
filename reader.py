#!/usr/bin/env python3
"""
RFID Access Control — читает UID с STM32/RC522 по USB CDC,
проверяет по списку, посылает команду OPEN, ждёт подтверждения RELAY1_OK.

Запуск:
    sudo python3 reader.py
    или добавьте пользователя в группу dialout и запускайте без sudo:
    sudo usermod -aG dialout $USER   (перелогиниться после этого)

Файл cards.txt — по одному UID на строку, регистр не важен:
    3B0AF705
    61 14 FF 5D        <- пробелы тоже допустимы, они отфильтруются
    # это комментарий
"""

import serial
import time
import logging
import sys
import os

# ── Настройки ──────────────────────────────────────────────────────────────
PORT       = "/dev/ttyACM0"
BAUD       = 115200
CARDS_FILE = "cards.txt"
LOG_FILE   = "access.log"

OPEN_CMD        = b"OPEN\n"
EXPECTED_ACK    = "RELAY1_OK"
ACK_TIMEOUT_SEC = 3.0
# ───────────────────────────────────────────────────────────────────────────

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s  %(levelname)-8s  %(message)s",
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler(LOG_FILE, encoding="utf-8"),
    ],
)
log = logging.getLogger(__name__)


def load_cards(path: str) -> set[str]:
    if not os.path.exists(path):
        log.warning("Файл '%s' не найден — создаю пустой.", path)
        open(path, "w").close()
        return set()

    cards = set()
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            uid = line.replace(" ", "").replace(":", "").upper()
            if len(uid) == 8 and all(c in "0123456789ABCDEF" for c in uid):
                cards.add(uid)
            else:
                log.warning("Некорректная строка в cards.txt: '%s' — пропускаю.", line)
    log.info("Загружено %d авторизованных карт из '%s'.", len(cards), path)
    return cards


def normalize_uid(raw: str) -> str | None:
    uid = raw.strip().replace(" ", "").replace(":", "").upper()
    if len(uid) == 8 and all(c in "0123456789ABCDEF" for c in uid):
        return uid
    return None


def send_open(ser: serial.Serial) -> bool:
    ser.reset_input_buffer()
    ser.write(OPEN_CMD)
    ser.flush()
    log.debug("Отправлена команда OPEN.")

    deadline = time.monotonic() + ACK_TIMEOUT_SEC
    while time.monotonic() < deadline:
        if ser.in_waiting:
            line = ser.readline().decode("ascii", errors="ignore").strip()
            if not line:
                continue
            log.debug("Получена строка: '%s'", line)
            if EXPECTED_ACK in line:
                return True
        time.sleep(0.02)
    return False


def main():
    # Загружаем whitelist один раз при старте
    cards = load_cards(CARDS_FILE)
    cards_mtime = os.path.getmtime(CARDS_FILE) if os.path.exists(CARDS_FILE) else 0

    log.info("Открываю порт %s @ %d бод…", PORT, BAUD)
    try:
        ser = serial.Serial(PORT, BAUD, timeout=0.5)
    except serial.SerialException as e:
        log.error("Не удалось открыть порт: %s", e)
        log.error("Попробуйте: sudo usermod -aG dialout $USER  (и перелогиниться)")
        sys.exit(1)

    log.info("Ожидание карт. Для выхода — Ctrl+C.")
    try:
        while True:
            if ser.in_waiting == 0:
                time.sleep(0.05)
                continue

            raw = ser.readline().decode("ascii", errors="ignore").strip()
            if not raw:
                continue

            # Служебные строки контроллера — просто выводим в debug
            if raw.startswith("[USER]") or raw.startswith("[DEBUG]"):
                log.debug("MCU: %s", raw)
                continue

            uid = normalize_uid(raw)
            if uid is None:
                log.debug("Нераспознанная строка: '%s'", raw)
                continue

            # Перезагружаем whitelist только если файл изменился с прошлого раза
            if os.path.exists(CARDS_FILE):
                mtime = os.path.getmtime(CARDS_FILE)
                if mtime != cards_mtime:
                    cards = load_cards(CARDS_FILE)
                    cards_mtime = mtime

            # ── Проверка UID ───────────────────────────────────────────────
            if uid in cards:
                log.info("✅  ДОСТУП РАЗРЕШЁН  UID=%s", uid)
                if send_open(ser):
                    log.info("🔓  Relay1 открыт, подтверждение получено.")
                else:
                    log.warning("⚠️  Команда OPEN отправлена, но подтверждение не получено.")
            else:
                log.warning("❌  ДОСТУП ЗАПРЕЩЁН  UID=%s", uid)

    except KeyboardInterrupt:
        log.info("Завершение по Ctrl+C.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
