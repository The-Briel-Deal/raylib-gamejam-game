from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import threading
import webbrowser

PORT = 8000

class Handler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        super().end_headers()

url = f"http://localhost:{PORT}/raylib-game-template.html"

# Open after the server begins listening.
threading.Timer(0.5, lambda: webbrowser.open(url)).start()

print(f"Serving at {url}")
ThreadingHTTPServer(("localhost", PORT), Handler).serve_forever()