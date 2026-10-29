// Execute the shipped page script against a small DOM fixture. This checks
// control flow and safe rendering, not browser layout or radio connectivity.
export async function checkDashboard(fs, vm) {
 const html=await fs.readFile(new URL('../firmware/esp32/dashboard.html',import.meta.url),'utf8');
 const source=html.match(/<script>([\s\S]*?)<\/script>/)[1];
 const all=[],ids={};
 class Element {
  constructor(tag='div'){this.tag=tag;this.children=[];this.value='';this.checked=false;this.disabled=false;this.className='';this.textContent='';this.classList={toggle(){}};all.push(this);}
  append(...values){this.children.push(...values);}
  replaceChildren(...values){this.children=values;}
  setAttribute(){}
  addEventListener(){}
  showModal(){this.open=true;}
  close(){this.open=false;}
  querySelectorAll(){return this.children.flatMap(e=>e.children||[]).filter(e=>e.tag==='input'&&e.checked);}
 }
 for(const match of html.matchAll(/id="([^"]+)"/g))ids[match[1]]=new Element();
 const fixture={online:true,revision:123,flags:11,epoch:846617365,active:0,ageMs:100,uptime:5000,rtcErrors:0,oledErrors:0,clockProgress:3,badFrames:0,sound:1,brightness:5,duration:500,token:'fixture',history:[],pending:false,operation:0,result:-1,resultRevision:0,
  slots:Array.from({length:8},(_,i)=>({name:'REMINDER '+(i+1),hour:6+i,minute:0,days:127,enabled:i<6?1:0}))};
 let response=fixture,request;
 const context=vm.createContext({document:{getElementById:id=>ids[id],createElement:t=>new Element(t),createTextNode:text=>({textContent:text}),querySelectorAll:()=>all.filter(e=>e.className.includes('needs-link'))},
  console,Date,AbortSignal,URLSearchParams,setInterval(){},fetch:async(path,options)=>{request={path,options};return {ok:true,json:async()=>structuredClone(response)}}});
 vm.runInContext(source,context);await new Promise(setImmediate);
 const assert=(condition,message)=>{if(!condition)throw Error(message);};
 assert(ids.scheduleRows.children.length===8,'eight authoritative slots rendered');
 assert(ids.trust.textContent.includes('check passed'),'trusted RTC status');
 fixture.slots[0].name='<b>BAD</b>';fixture.revision++;
 vm.runInContext('poll()',context);await new Promise(setImmediate);
 assert(ids.scheduleRows.children[0].children[0].textContent==='<b>BAD</b>','names rendered as text');
 vm.runInContext('edit(0)',context);assert(ids.editor.open,'slot editor opens');
 assert(ids.days.children.length===7,'weekday editor renders seven days');
 response={operation:42};await vm.runInContext("send('/api/settings',{brightness:6,sound:1,duration:500},124)",context);
 assert(request.options.headers['X-Medbox-Token']==='fixture','POST has session token');
 assert(request.options.body.get('revision')==='124','draft revision retained');
 fixture.operation=42;fixture.pending=false;fixture.result=0;fixture.resultRevision=125;fixture.revision=125;fixture.brightness=6;response=fixture;
 await vm.runInContext('poll()',context);assert(ids.message.textContent.includes('read back'),'success after ACK and readback');
 fixture.online=false;await vm.runInContext('poll()',context);assert(ids.connection.textContent==='STM32 offline','offline state explicit');
 assert(ids.saveSlot.disabled,'offline save disabled');
 return 'PASS: dashboard script, eight slots, safe text, weekday editor, token/revision POST, ACK/readback success, offline controls';
}
