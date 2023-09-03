
// V03.05.2020
//
var befehllist=[{b:'LEDWLANON',n:"On"},{b:'LEDWLANOFF',n:"Off"}];//b=befehn n=beschriftungsname
var typen={
	'ledWLAN':	{a:false,n:'LED (Wlan)',b:['LEDWLANON','LEDWLANOFF']}
};
var IOGroup,thebendercom,espdata={};
var onError=function(){
	if(IOGroup)IOGroup.error();
	addClass(gE("sysinfo"),"error");
	addClass(gE("filelist"),"error");
	addClass(gE("actions"),"error");
}
var onOK=function(){
	subClass(gE("sysinfo"),"error");
	subClass(gE("filelist"),"error");	
	subClass(gE("actions"),"error");
}
var getpostData =function(url, auswertfunc,POSTdata,noheader,rh){
		var loader,i;
		try {loader=new XMLHttpRequest();}
		catch(e){
				try{loader=new ActiveXObject("Microsoft.XMLHTTP");}
				catch(e){
					try{loader=new ActiveXObject("Msxml2.XMLHTTP");}
					catch(e){loader=null;}
				}
			}	
		if(!loader)alert('XMLHttp nicht möglich.');
		var jdata=undefined;
		if(POSTdata!=undefined)jdata=POSTdata;//encodeURI
		
		loader.onreadystatechange=function(){
			if(loader.readyState==4){
				auswertfunc(loader);
				onOK();
				}
			};
		loader.ontimeout=function(e){console.log("TIMEOUT");}
		loader.onerror=function(e){console.log("ERR",e,loader.readyState);onError();}
		
		if(jdata!=undefined){
				loader.open("POST",url,true);
				if(rh!=undefined){
						for(i=0;i<rh.length;i++){
							loader.setRequestHeader(rh[i].typ,rh[i].val);
						}
				}
				if(noheader!==true){
					//loader.responseType="text";
					loader.setRequestHeader("Content-Type","application/x-www-form-urlencoded");
					//loader.setRequestHeader("Content-Type","text/plain");
					loader.setRequestHeader('Cache-Control','no-cache');
					loader.setRequestHeader("Pragma","no-cache");
					loader.setRequestHeader("Cache-Control","no-cache");
					jdata=encodeURI(POSTdata);
				}
				loader.send(jdata);
			}
			else{
				loader.open('GET',url,true);
				loader.setRequestHeader('Content-Type', 'text/plain');
				loader.send(null);
			}
	}
var cE=function(ziel,e,id,cn){
	var newNode=document.createElement(e);
	if(id!=undefined)newNode.id=id;
	if(cn!=undefined)newNode.className=cn;
	if(ziel)ziel.appendChild(newNode);
	return newNode;
	}
var gE=function(id){return document.getElementById(id);}
var addClass=function(htmlNode,Classe){	
	var newClass;
	if(htmlNode!=undefined){
		newClass=htmlNode.className;
		if(newClass==undefined || newClass=="")newClass=Classe;
		else
		if(!istClass(htmlNode,Classe))newClass+=' '+Classe;	
		htmlNode.className=newClass;
	}			
}
var subClass=function(htmlNode,Classe){
		var aClass,i;
		if(htmlNode!=undefined && htmlNode.className!=undefined){
			aClass=htmlNode.className.split(" ");	
			var newClass="";
			for(i=0;i<aClass.length;i++){
				if(aClass[i]!=Classe){
					if(newClass!="")newClass+=" ";
					newClass+=aClass[i];
					}
			}
			htmlNode.className=newClass;
		}
}
var istClass=function(htmlNode,Classe){
	if(htmlNode.className){
		var i,aClass=htmlNode.className.split(' ');
		for(i=0;i<aClass.length;i++){
				if(aClass[i]==Classe)return true;
		}	
	}		
	return false;
}
var filterJSON=function(s){
	var re=s;
	if(re.indexOf("'")>-1)re=re.split("'").join('"');
	try {re=JSON.parse(re);} 
	catch(e){
		console.log("JSON.parse ERROR:",s,":");
		re={"error":"parseerror"};
		}
	return re;
}
var cButt=function(z,txt,cl,data,click){
	var a=cE(z,"a");
	a.className=cl;
	a.innerHTML=txt;
	a.href="#";
	a.data=data;
	a.addEventListener('click', click);
	return a;
};

var getMouseP=function(e){
return{
	x:document.all ? window.event.clientX : e.pageX,
	y:document.all ? window.event.clientY : e.pageY};
}
var getPos=function(re,o){
	var r=o.getBoundingClientRect();
	re.x-=r.left;
	re.y-=r.top;
	return re;
}
var relMouse=function(e,o){
	return getPos(getMouseP(e),o);
}
var absMouse=function(e,o){
	var p=getPos(getMouseP(e),o);
	var scrollLeft = window.pageXOffset || document.documentElement.scrollLeft,
	    scrollTop = window.pageYOffset || document.documentElement.scrollTop;
	p.x-=scrollLeft;
	p.y-=scrollTop;
	console.log(e,scrollLeft,scrollTop);
	return p;
}

var oIOGroup=function(){
	var ESP8266URL="./action",
		isinit=false;
	var txtinput;
	var txtantwort;
	var lasttimestamp="";
	
	this.refresh=function(data){refresh(data);}
	this.error=function(){}
	
	
	var addtotextfeld=function(s){
		txtantwort.innerHTML+=s+'<br>';
		txtantwort.scrollTop =txtantwort.scrollHeight;
	}
	
	var kopfclick=function(e){
		txtinput.value="kopf "+this.data.pos;
		clicksend(e);
		e.preventDefault();
	}
	
	var auge;
	var augenclick=function(e){
		var pos=absMouse(e,this);
		pos.x=Math.floor(pos.x/5)*5;
		pos.y=Math.floor(pos.y/5)*5;
		console.log(pos);
		txtinput.value="augen "+(100-pos.x)+" "+pos.y;
		auge.style.left=pos.x+'px';
		auge.style.top=pos.y+'px';
		clicksend(e);
		e.preventDefault();
	}
	
	var mundkey=function(e){
		if(e.keyCode==13){
			txtinput.value="say:"+this.value;
			this.value="";
			clicksend(e);
		}
		e.preventDefault();
	}
	
	var armclick=function(e){
		var pos=absMouse(e,this);
		pos.x=Math.floor(pos.x/5)*5;
		pos.y=Math.floor(pos.y/5)*5;
		console.log(pos,this.data.arm);
		
		txtinput.value=this.data.arm+(100-pos.y);
		clicksend(e);
		e.preventDefault();
	}
	
	var createRegler=function(ziel){
		//kopf
		//augen
		//arme
		var i,a,kp,inp,
			node=cE(ziel,"span",undefined,"kopf");
		kp=cE(node,"span",undefined,"kopfpos");
		var max=9;
		//0 . 25 . 50 . 75 . 100
		for(i=0;i<max;i++){
			a=cE(kp,"a");
			a.data={pos:100-Math.round(100/(max-1)*(i))};
			a.href="#";
			a.title=a.data.pos;
			a.addEventListener("click",kopfclick);
		}
		
		kp=cE(node,"span",undefined,"augenpos");
		kp.addEventListener("click",augenclick);
		auge=cE(kp,"span",undefined,"auge");
		
		inp=cE(node,"input",undefined,"mund");
		inp.type = "text";
		inp.placeholder = "say:";
		inp.addEventListener("keyup",mundkey);
		
		kp=cE(node,"span",undefined,"arml");
		kp.data={arm:"arm links "};
		kp.addEventListener("click",armclick);
		kp=cE(node,"span",undefined,"armr");
		kp.data={arm:"arm rechts "};
		kp.addEventListener("click",armclick);
	}
	
	
	var refresh=function(data){
		if(!isinit)create(data);
		
		for(param in data.portstatus){ 
			o=typen[param];
			if(o && o.a){
				if(data.portstatus[param]){
					subClass(o.ostat,"inaktiv");
					subClass(o.obutt,"txtan");
					addClass(o.obutt,"txtaus");
					}
				else{
					addClass(o.ostat,"inaktiv");
					addClass(o.obutt,"txtan");
					subClass(o.obutt,"txtaus");
					}
			}
		}
		if(data.antwort){
			var s=data.antwort.text,ti=data.antwort.t;
			if(lasttimestamp!==ti){				
				if(s.indexOf('bchat')>-1){					
					if(s.indexOf('(')>-1){
						var ip=s.split('(')[1].split(')')[0];
						if(ip==espdata.remoteIP){//direkte Ansprache
							addtotextfeld(''+ti+' <b>Bender:</b>'+s.split(':')[1]+'');
						}
					}
					else
					   addtotextfeld(ti+' Bender:'+s.split(':')[1]);//ohne IP			
				}
				else
					addtotextfeld('<span class="sysmsg">'+ti+' system:'+s+'<span>');
				
			}
			lasttimestamp=data.antwort.t;
		}
	}
	var create=function(data){
		var param,z,o,b,i,p,h2;
		z=gE("actions");
		if(z && data.portstatus!=undefined){
			for(param in data.portstatus){ 
				if(typen[param])typen[param].a=true;//aktivieren	
			}
			//Interaktionen
			for(param in typen){
				o=typen[param];
				if(o.a){
					p=cE(z,"p");
					
					o.ostat=cE(p,"span",undefined,"pout c"+param+" inaktiv");					
					o.obutt =cButt(p,'',"butt",o.b,buttclick);
					
					h2=cE(p,"h2");
					h2.innerHTML=o.n;
				}
			}
			
			//inputs(serielle to myphy)
			p=cE(z,"p");
			txtinput=cE(p,'input');
			txtinput.type="text";
			txtinput.setAttribute("size",20);
			txtinput.addEventListener('keyup',clickkey);
			
			b=cButt(p,'senden',"butt","",clicksend);
			
			p=cE(z,"p");
			b=cButt(p,'say:hallo',"butt","say:hallo",clicksay);
			b=cButt(p,'wie spät',"butt","wie spät",clicksay);
			b=cButt(p,'wie lange',"butt","wie lange",clicksay);
			b=cButt(p,'welcher tag',"butt","welcher tag",clicksay);
			b=cButt(p,'zähle von 10',"butt","zähle von 10",clicksay);
			b=cButt(p,'mute',"butt","mute",clicksay);
			b=cButt(p,'unmute',"butt","unmute",clicksay);
			b=cButt(p,'armL up',"butt","arm links hoch",clicksay);
			b=cButt(p,'armL down',"butt","arm links runter",clicksay);
			b=cButt(p,'armR up',"butt","arm rechts hoch",clicksay);
			b=cButt(p,'armR down',"butt","arm rechts runter",clicksay);
			b=cButt(p,'aniaus',"butt","aniaus",clicksay);
			b=cButt(p,'anian',"butt","anian",clicksay);
			
			b=cButt(p,'augen 50 50',"butt","augen 50 50",clicksay);
			b=cButt(p,'augen 0 50',"butt","augen 0 50",clicksay);
			b=cButt(p,'augen 100 50',"butt","augen 100 50",clicksay);
			b=cButt(p,'kopf 50',"butt","kopf 50",clicksay);
			
			//b=cButt(p,'say:hallo',"butt","say:hallo",clicksay);
			p=cE(z,"p",undefined,"pkopf");
			createRegler(p);
			
			p=cE(z,"p");
			txtantwort=cE(p,'div',undefined,'chatmsg');
			//txtantwort.setAttribute("rows",10);
			//txtantwort.setAttribute("readonly","");
			
			isinit=true;
		}
	}
	
	var clicksay=function(e){
		txtinput.value=this.data;
		//if(this.data)
		clicksend(e);
	}
	
	var clickkey=function(e){
		if(e.keyCode==13)clicksend(e);
	}
	
	var clicksend=function(e){
		var d = new Date();
		var ti="";
		var h=d.getHours(),m=d.getMinutes(),s=d.getSeconds();
		if(h<10)ti+="0"
		ti+=h+':';
		if(m<10)ti+="0"
		ti+=m+':';
		if(s<10)ti+="0"
		ti+=s;
		addtotextfeld('<i>'+ti+' Du:'+txtinput.value+'</i>');
		
		getpostData(ESP8266URL+"?benderser="+txtinput.value,fresult);
		txtinput.value="";
		e.preventDefault();
	}
	
	
		
	var buttclick=function(e){
		if(istClass(this,"txtan")){
			subClass(this,"txtan");
			addClass(this,"txtaus");
			getpostData(ESP8266URL+"?bender="+this.data[0],fresult);
		}
		else{
			addClass(this,"txtan");
			subClass(this,"txtaus");
			getpostData(ESP8266URL+"?bender="+this.data[1],fresult);
		}
		e.preventDefault();
	}	
	var fresult=function(data){
		var j=filterJSON(data.responseText);
		console.log(j);//befehl:"ok"
		//get status
		getpostData("./data.json",function(d){refresh(filterJSON(d.responseText))});
	}
}
var bendercom=function(){
	var dateisysinfo="./data.json";
	var ziel;
	var savetimeout=undefined;
	var timerlistreload=undefined;
	var lokdat=undefined;
	
	var wochentag=["Montag","Dienstag","Mittwoch","Donnerstag","Freitag","Samstag","Sonntag"];
	
	this.error=function(){}
	
	var uploadtimerdaten=function(s){}	
		
	var anzinputs=0;
	var cI=function(ziel,typ,value,title){//create input
		var label;
		var input=cE(ziel,"input");
		input.type=typ;
		if(typ=="checkbox"){
			input.checked=value;
			input.id="cb"+anzinputs;
			label=cE(ziel,"label");
			label.htmlFor=input.id;
			input.dataLabel=label;
		}	
		else
			input.value=value;
		if(title!=undefined)input.title=title;	
		anzinputs++;
		return input;
	}
	var addBefehle=function(node,dat){
		var i,o;
		for(i=0;i<befehllist.length;i++){
			o=cE(node,"option");
			o.value=befehllist[i].b;
			o.innerHTML=befehllist[i].n;
		}
	}
	
		
	var changetimekorr=function(e){
		var val=this.value;//in Stunden
		getpostData(dateisysinfo+'?settimekorr='+val,
			function(d){
				//console.log('reload data',d);
				setTimeout(function(){
					getpostData(dateisysinfo,fresultsysinfo);
					}
				,1000);//1 sec warten, bis intern Zeit gesetzt wurde
			}
		);
	}
	
	var fresultsysinfo=function(data){
		var ziel=gE('sysinfo'),
			jdat=filterJSON(data.responseText),
			div,node,p,a,s;
		if(ziel){
			ziel.innerHTML="";
			if(jdat.datum==undefined)return;
			
			div=cE(ziel,"div",undefined,"utctimset");
			div.innerHTML="UTC Zeitunterschied:";
			var val=Math.floor(jdat.datum.timekorr);
			node=cI(div,"number",val,"Zeitunterschied");
			node.addEventListener('change',changetimekorr);
			node.maxlength=2;
			node.size=2;
			if(val==-1 || val==1)
				node=document.createTextNode(" Stunde");
			else
				node=document.createTextNode(" Stunden");
			div.appendChild(node);
			
			lokdat=cE(ziel,"article");
			getlokaldata(jdat);
			
			node=document.getElementsByTagName('h1')[0];
			if(node)
				node.innerHTML=jdat.progversion;
			
		}
	}
	var retlokaldata=function(data){
		jdat=filterJSON(data.responseText);
		getlokaldata(jdat);
	}
	var iftimr;
	var getlokaldata=function(jdat){
		var node;
		if(lokdat!=undefined){
			if(iftimr!=undefined)clearTimeout(iftimr);
			
			if(jdat.error!=undefined){
				console.log("Fehler",typeof jdat,jdat);
				iftimr=setTimeout(function(){
					getpostData(dateisysinfo,retlokaldata);
				},1000*20);//20sec
				return;
			}
			
			lokdat.innerHTML="";			
			node=cE(lokdat,"p");
			var t=jdat.lokalzeit.split(":");
			node.innerHTML="lokaltime: "+t[0]+':'+t[1];
			
			node=cE(lokdat,"p");
			s="";
			if(jdat.datum.day<10)s+="0";
			s+=jdat.datum.day+".";
			if(jdat.datum.month<10)s+="0";
			s+=jdat.datum.month+".";
			node.innerHTML="Datum: "+s+jdat.datum.year+" "+wochentag[jdat.datum.tag];
						
			node=cE(lokdat,"p");
			node.innerHTML="Sommerzeit: "+jdat.datum.summertime;
			
			node=cE(lokdat,"p");
			node.innerHTML="MAC: <span style=\"text-transform: uppercase;\">"+jdat.macadresse+"</span>";
			
			if(jdat.remoteIP){
				node=cE(lokdat,"p");
				node.innerHTML="cIP: <span style=\"text-transform: uppercase;\">"+jdat.remoteIP+"</span>";
			}
			espdata=jdat;
			
			if(IOGroup)IOGroup.refresh(jdat);
			
			
			iftimr=setTimeout(function(){
				getpostData(dateisysinfo,retlokaldata);
			},1000*2);//2sec -> chat!
		}
	}
	
	var ini=function(){
		var z2=gE('sysinfo');
		if(z2)getpostData(dateisysinfo,fresultsysinfo);//+'&time='+tim.getTime()
	}
	
	ini();
}
window.addEventListener('load', function (event) {
	IOGroup=new oIOGroup();
	thebendercom=new bendercom();
});