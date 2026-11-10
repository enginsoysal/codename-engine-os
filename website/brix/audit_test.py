"""Exercise real HTTP redirect/error behavior of the public link auditor."""
import threading,unittest
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from audit import fetch

class Handler(BaseHTTPRequestHandler):
    def log_message(self,*args):pass
    def do_GET(self):
        if self.path in ('/old','/loop'):
            self.send_response(308);self.send_header('Location','/new' if self.path=='/old' else '/loop');self.end_headers()
        elif self.path=='/new':
            self.send_response(200);self.end_headers();self.wfile.write(b'<html lang="en"><title>Engacer OS</title><body>Guide</body></html>')
        else:self.send_response(404);self.end_headers()

class AuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server=ThreadingHTTPServer(('127.0.0.1',0),Handler)
        cls.thread=threading.Thread(target=cls.server.serve_forever,daemon=True);cls.thread.start()
        cls.base='http://127.0.0.1:'+str(cls.server.server_port)
    @classmethod
    def tearDownClass(cls):cls.server.shutdown();cls.server.server_close();cls.thread.join()
    def test_permanent_redirect_reaches_the_actual_page(self):
        r=fetch(self.base+'/old');self.assertEqual(r['status'],200);self.assertEqual(r['final_url'],self.base+'/new')
    def test_redirect_loop_remains_a_failure(self):self.assertEqual(fetch(self.base+'/loop')['status'],308)
    def test_missing_target_remains_a_failure(self):self.assertEqual(fetch(self.base+'/missing')['status'],404)

if __name__=='__main__':unittest.main()
