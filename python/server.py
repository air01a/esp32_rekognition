#!/usr/bin/env python3
 

TOKEN = ""
TIMER = 30

__all__ = ["SimpleHTTPRequestHandler"]
 
import os
import posixpath
import http.server
import urllib.request, urllib.parse, urllib.error
import cgi
import shutil
import mimetypes
import re
from io import BytesIO
import time 
from AWSRekognition import AWSRekognition 
import re
import np
import cv2

class SimpleHTTPRequestHandler(http.server.BaseHTTPRequestHandler):
	# check for security token
	def secure(self):
		global TOKEN
		self.cookie='?'
		headers = self.headers.get('Authorization')
		if  headers==None:
			print(str(self.path))
			if str(self.path).find(TOKEN)!=-1:
			   self.cookie='?id=' + TOKEN 
			   return True
		if headers == TOKEN:
			return True
        
		self.send_response(503)
		self.end_headers()
		return False

	#Manage GET
	def do_GET(self):
		if not self.secure():
			return False

		"""Serve a GET request."""
		f = self.send_head()
		if f:
			self.copyfile(f, self.wfile)
			f.close()
	
	# Manage HEAD
	def do_HEAD(self):
		if not self.secure():
			return False
		"""Serve a HEAD request."""
		f = self.send_head()
		if f:
			f.close()
 
	# Mange POST to get FILE
	def do_POST(self):
		if not self.secure():
			return False

		"""Serve a POST request."""
		r, info = self.deal_post_data()
		print((r, info, "by: ", self.client_address))
		f = BytesIO()
		f.write(str(TIMER).encode())
		length = f.tell()
		f.seek(0)
		self.send_response(200)
		self.send_header("Content-type", "text/html")
		self.send_header("Content-Length", str(length))
		self.end_headers()
		if f:
			self.copyfile(f, self.wfile)
			f.close()
		
	# Use AWS Reko to draw bbox around people on the frame
	def getHuman(self,frame):
		aws = AWSRekognition()
		res = aws.labelDetection(frame)
		print(res)
		file_bytes = np.asarray(bytearray(frame), dtype=np.uint8)
		img = cv2.imdecode(file_bytes, cv2.IMREAD_COLOR)
		for box in res:
			cat,prob,x,y,w,h,module = box
			if cat=='Person':
				cv2.rectangle(img, (int(x-w/2), int(y-h/2)), (int(x+w/2), int(y+h/2)), (255, 0, 0))
		name=str(time.time())+".jpg"
		cv2.imwrite(name, img)


	# Get file in POST DATA the Ugly way
	def deal_post_data(self):
		content_type = self.headers['content-type']
		if not content_type:
			return (False, "Content-Type header doesn't contain boundary")
		boundary = content_type.split("=")[1].encode()
		remainbytes = int(self.headers['content-length'])
		line = self.rfile.readline()
		remainbytes -= len(line)
		if not boundary in line:
			return (False, "Content NOT begin with boundary")
		line = self.rfile.readline()
		remainbytes -= len(line)
		fn = re.findall(r'Content-Disposition.*name="imageFile"; filename="(.*)"', line.decode())
		if not fn:
			return (False, "Can't find out file name...")
		path = self.translate_path(self.path)
		fn = os.path.join(path, fn[0])
		line = self.rfile.readline()
		remainbytes -= len(line)
		line = self.rfile.readline()
		remainbytes -= len(line)

		out = BytesIO()
		preline = self.rfile.readline()
		remainbytes -= len(preline)
		while remainbytes > 0:
			line = self.rfile.readline()
			remainbytes -= len(line)
			if boundary in line:
				preline = preline[0:-1]
				if preline.endswith(b'\r'):
					preline = preline[0:-1]
				out.write(preline)
				self.getHuman(out.getvalue())
				return (True, "File '%s' upload success!" % fn)
			else:
				out.write(preline)
				preline = line

		return (False, "Unexpect Ends of data.")
 


	# Send header to get and head request
	def send_head(self):
		path = self.translate_path(self.path)
		f = None
		if os.path.isdir(path):
			if not self.path.endswith('/'):
				# redirect browser - doing basically what apache does
				self.send_response(301)
				self.send_header("Location", self.path + "/")
				self.end_headers()
				return None
			for index in "index.html", "index.htm":
				index = os.path.join(path, index)
				if os.path.exists(index):
					path = index
					break
			else:
				return self.list_directory(path)
		ctype = self.guess_type(path)
		try:
			f = open(path, 'rb')
		except IOError:
			self.send_error(404, "File not found")
			return None
		self.send_response(200)
		self.send_header("Content-type", ctype)
		fs = os.fstat(f.fileno())
		self.send_header("Content-Length", str(fs[6]))
		self.send_header("Last-Modified", self.date_time_string(fs.st_mtime))
		self.end_headers()
		return f
 

	# List files in current directory and encapsulate the result in html
	def list_directory(self, path):	
		try:
			list = os.listdir(path)
		except os.error:
			self.send_error(404, "No permission to list directory")
			return None
		list.sort(key=lambda a: a.lower())
		f = BytesIO()
		displaypath = cgi.escape(urllib.parse.unquote(self.path))
		f.write(b'<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">')
		f.write(("<html>\n<title>Directory listing for %s</title>\n" % displaypath).encode())
		f.write(("<body>\n<h2>Directory listing for %s</h2>\n" % displaypath).encode())
		f.write(b"<hr>\n")
		f.write(b"<form ENCTYPE=\"multipart/form-data\" method=\"post\">")
		f.write(b"<input name=\"imageFile\" type=\"file\"/>")
		f.write(b"<input type=\"submit\" value=\"upload\"/></form>\n")
		f.write(b"<hr>\n<ul>\n")
		for name in list:
			fullname = os.path.join(path, name)
			displayname = linkname = name
			# Append / for directories or @ for symbolic links
			if os.path.isdir(fullname):
				displayname = name + "/"
				linkname = name + "/"
			if os.path.islink(fullname):
				displayname = name + "@"
				# Note: a link to a directory displays with @ and links with /
			f.write(('<li><a href="%s">%s</a>\n'
					% (urllib.parse.quote(linkname)+self.cookie, cgi.escape(displayname))).encode())
		f.write(b"</ul>\n<hr>\n</body>\n</html>\n")
		length = f.tell()
		f.seek(0)
		self.send_response(200)
		self.send_header("Content-type", "text/html")
		self.send_header("Content-Length", str(length))
		self.end_headers()
		return f
 
	def translate_path(self, path):
		# abandon query parameters
		path = path.split('?',1)[0]
		path = path.split('#',1)[0]
		path = posixpath.normpath(urllib.parse.unquote(path))
		words = path.split('/')
		words = [_f for _f in words if _f]
		path = os.getcwd()
		for word in words:
			drive, word = os.path.splitdrive(word)
			head, word = os.path.split(word)
			if word in (os.curdir, os.pardir): continue
			path = os.path.join(path, word)
		return path
 
	def copyfile(self, source, outputfile):
		shutil.copyfileobj(source, outputfile)
 
	def guess_type(self, path):
		base, ext = posixpath.splitext(path)
		if ext in self.extensions_map:
			return self.extensions_map[ext]
		ext = ext.lower()
		if ext in self.extensions_map:
			return self.extensions_map[ext]
		else:
			return self.extensions_map['']
 
	if not mimetypes.inited:
		mimetypes.init() # try to read system mime.types
	extensions_map = mimetypes.types_map.copy()
	extensions_map.update({
		'': 'application/octet-stream', # Default
		'.py': 'text/plain',
		'.c': 'text/plain',
		'.h': 'text/plain',
		})
 
 
def run(HandlerClass = SimpleHTTPRequestHandler,ServerClass = http.server.HTTPServer):
	server_address = ('0.0.0.0', 8081)
	httpd = ServerClass(server_address,HandlerClass)
	httpd.serve_forever()
 
if __name__ == '__main__':
	run()
