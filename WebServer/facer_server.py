import web
import threading
import time

filename = "detectedFaces"
timewindow = 2
refreshrate = 30


urls = (
    '/', 'hello'
)
app = web.application(urls, globals())

class hello:
    def GET(self):
        try:
            file = open(filename)
            lines = file.readlines()
            length = len(lines)
        finally:
            file.close()
        lastunixtime = int( (lines[length-1]).split(',')[0] )
        start = length - (30*timewindow)
        if start < 0:
            start = 0
        set_names = set()
        for i in range(start, length):
            l = lines[i].replace(',','').split()
            if (lastunixtime - int(l[0])) < timewindow:
                if len(l) > 1:
                    for n in range(1, len(l)):
                        set_names.add(l[n])
        detected_names = ', '.join(set_names)
        return detected_names

if __name__ == "__main__":
    app.run()
