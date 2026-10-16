from html.parser import HTMLParser
from pathlib import Path

t = Path("index.html").read_text(encoding="utf-8")
HTMLParser().feed(t)
print("bytes", len(t.encode("utf-8")))
print("title_ok", "Device Monitor" in t)
print("view_count", t.count('class="view'))
print("script_ok", "execute(" in t)
