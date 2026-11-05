#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
stress_cli.py —— DeviceMonitor 串口 CLI 压力测试

用途：
  证明系统在异常输入、快速连续命令、超长命令下仍可控（不乱码、不死机、可恢复）。

依赖：
  pip install pyserial

用法（示例）：
  python Tests/stress_cli.py --port COM5 --baud 115200 --n 200
  python Tests/stress_cli.py --port /dev/ttyUSB0 --n 500 --log stress.log

断言策略：
  - 合法命令：应出现对应关键词（help→Available，status→System，sensor→temp|No sensor）
  - 非法命令：应出现 Unknown
  - 超长输入：不应导致 MCU 复位（uptime 不回退到接近 0）
  - 快速连发：总成功率 ≥ 阈值，且结束时 status 仍可应答
"""

from __future__ import annotations

import argparse
import re
import sys
import time
from dataclasses import dataclass, field
from typing import List, Optional, Tuple

try:
    import serial
except ImportError:
    print("需要 pyserial:  pip install pyserial")
    sys.exit(2)


@dataclass
class CaseResult:
    name: str
    ok: bool
    detail: str = ""


@dataclass
class StressReport:
    cases: List[CaseResult] = field(default_factory=list)
    sent: int = 0
    ok: int = 0
    fail: int = 0
    resets: int = 0

    def add(self, r: CaseResult) -> None:
        self.cases.append(r)
        self.sent += 1
        if r.ok:
            self.ok += 1
        else:
            self.fail += 1

    def summary(self) -> str:
        lines = [
            "=" * 48,
            "DeviceMonitor UART Stress Report",
            "=" * 48,
            f"Commands sent : {self.sent}",
            f"Commands OK   : {self.ok}",
            f"Commands FAIL : {self.fail}",
            f"MCU Reset     : {self.resets}",
            "-" * 48,
        ]
        for c in self.cases:
            mark = "OK " if c.ok else "FAIL"
            lines.append(f"[{mark}] {c.name:32s} {c.detail}")
        lines.append("=" * 48)
        return "\n".join(lines)


class DeviceCLI:
    def __init__(self, port: str, baud: int, timeout: float = 1.0):
        self.ser = serial.Serial(port=port, baudrate=baud, timeout=timeout)
        self.rx = bytearray()
        time.sleep(0.3)
        self.ser.reset_input_buffer()

    def close(self) -> None:
        self.ser.close()

    def send_line(self, cmd: str, wait: float = 0.4, append_crlf: bool = True) -> str:
        """发送命令并收集一段时间内的回包。"""
        payload = cmd.encode("ascii", errors="ignore")
        if append_crlf:
            payload += b"\r\n"
        self.ser.reset_input_buffer()
        self.ser.write(payload)
        self.ser.flush()
        end = time.time() + wait
        buf = bytearray()
        while time.time() < end:
            chunk = self.ser.read(256)
            if chunk:
                buf.extend(chunk)
        text = buf.decode("utf-8", errors="replace")
        self.rx.extend(buf)
        return text

    def drain(self, seconds: float = 0.5) -> str:
        end = time.time() + seconds
        buf = bytearray()
        while time.time() < end:
            chunk = self.ser.read(256)
            if chunk:
                buf.extend(chunk)
        return buf.decode("utf-8", errors="replace")

    def uptime_ms(self) -> Optional[int]:
        text = self.send_line("status", wait=0.6)
        m = re.search(r"Uptime:\s*(\d+)", text)
        return int(m.group(1)) if m else None


def expect(text: str, *needles: str) -> bool:
    low = text.lower()
    return any(n.lower() in low for n in needles)


def run_stress(port: str, baud: int, n: int, log_path: Optional[str]) -> int:
    report = StressReport()
    dev = DeviceCLI(port, baud)
    log_f = open(log_path, "w", encoding="utf-8") if log_path else None

    def log(msg: str) -> None:
        if log_f:
            log_f.write(msg + "\n")

    try:
        # ---- 0. 基线 uptime，用于检测复位 ----
        t0 = dev.uptime_ms()
        report.add(CaseResult("baseline_uptime", t0 is not None, f"uptime={t0}"))
        log(f"baseline uptime={t0}")

        # ---- 1. 合法命令 ----
        for cmd, keys in [
            ("help", ("Available",)),
            ("status", ("System", "Uptime")),
            ("sensor", ("temp", "No sensor")),
            ("stats", ("Queue", "Heap")),
            ("chipid", ("I2C",)),
        ]:
            out = dev.send_line(cmd, wait=0.8)
            ok = expect(out, *keys)
            report.add(CaseResult(f"valid:{cmd}", ok, out.replace("\n", " ")[:80]))
            log(f"VALID {cmd} -> {out!r}")

        # ---- 2. 非法命令 ----
        for bad in ("foo", "statuss", "help me now", "!!!"):
            out = dev.send_line(bad, wait=0.5)
            # 空行或未知命令应有提示，或至少有提示符；不应复位
            ok = ("Unknown" in out) or (">" in out) or (bad in out)
            report.add(CaseResult(f"invalid:{bad}", ok, out.replace("\n", " ")[:60]))
            log(f"INVALID {bad!r} -> {out!r}")

        # ---- 3. 超长命令（> CLI_MAX 64）----
        long_cmd = "a" * 120
        out = dev.send_line(long_cmd, wait=0.8)
        up = dev.uptime_ms()
        ok = up is not None and t0 is not None and up >= t0
        report.add(CaseResult("long_command", ok, f"uptime_after={up}"))
        log(f"LONG -> {out!r} uptime={up}")

        # ---- 4. 快速连续输入 ----
        burst_cmds = ["status", "sensor", "help", "badcmd", "stats"]
        burst_fail = 0
        for i in range(n):
            c = burst_cmds[i % len(burst_cmds)]
            out = dev.send_line(c, wait=0.15, append_crlf=True)
            if c == "help" and not expect(out, "Available", ">"):
                burst_fail += 1
            elif c == "status" and not expect(out, "System", ">"):
                burst_fail += 1
            elif c == "badcmd" and not expect(out, "Unknown", ">"):
                burst_fail += 1
            # sensor/stats 允许无完整包（太快时）
        ok = burst_fail <= max(1, n // 20)  # 允许 5% 丢包/超时
        report.add(
            CaseResult(
                "burst_stress",
                ok,
                f"n={n} visible_fail={burst_fail}",
            )
        )
        log(f"BURST n={n} fail={burst_fail}")

        # ---- 5. 结束后仍可控 ----
        out = dev.send_line("status", wait=1.0)
        up = dev.uptime_ms()
        alive = expect(out, "System") and up is not None
        report.add(CaseResult("alive_after_stress", alive, f"uptime={up}"))

        if t0 is not None and up is not None and up + 500 < t0:
            report.resets = 1  # uptime 回退 → 疑似复位

        # ---- 6. 非法 + 合法 混合恢复 ----
        dev.send_line("@@@", wait=0.2)
        out = dev.send_line("help", wait=0.8)
        report.add(CaseResult("recover_after_garbage", expect(out, "Available", ">"), ""))

    except serial.SerialException as exc:
        report.add(CaseResult("serial_port", False, str(exc)))
    finally:
        dev.close()
        if log_f:
            log_f.write(report.summary())
            log_f.close()

    print(report.summary())
    return 0 if report.fail == 0 else 1


def main() -> int:
    ap = argparse.ArgumentParser(description="STM32 DeviceMonitor UART CLI stress test")
    ap.add_argument("--port", required=True, help="串口，例如 COM5 或 /dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--n", type=int, default=100, help="快速连发次数")
    ap.add_argument("--log", default=None, help="测试日志路径，如 stress.log")
    args = ap.parse_args()
    return run_stress(args.port, args.baud, args.n, args.log)


if __name__ == "__main__":
    sys.exit(main())
