setInterval(function(){
	var xhr = new XMLHttpRequest();
	xhr.open("GET", "http://localhost:8000/", true);
	xhr.onreadystatechange = function(){ 
		if (xhr.readyState==4){on_load(xhr)}
    }
	xhr.send("");
},5000);

function on_load(request){
    res = request.responseText
    if (res.length > 0) {
        var xhrp = new XMLHttpRequest();
        xhrp.open("POST", "http://192.168.1.120:8080/watch", true);
        xhrp.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
        xhrp.send("names=" + res + "&uri=" + location.href);
    }
}