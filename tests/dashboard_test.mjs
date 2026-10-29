// Execute the shipped page script against a small DOM fixture. This checks
// control flow and safe rendering, not browser layout or radio connectivity.
export async function checkDashboard(fs, vm) {
 const html=await fs.readFile(new URL('../firmware/esp32/dashboard.html',import.meta.url),'utf8');
 const source=html.match(/<script>([\s\S]*?)<\/script>/)[1];
 const all=[],ids={};
 class Element {
  constructor(tag='div'){this.tag=tag;this.children=[];this.value='';this.checked=false;this.disabled=false;this.className='';this.textContent='';this.style={};this.classList={toggle(){}};all.push(this);}
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
 const metrics={available:false,generation:5,recordCount:2,intervalCount:1,trackingSince:846600000,totalOpens:3,extraOpens:1,boundary:846617365,intervalOpens:1,intervalExtra:0,intervalPartial:0,untrustedOpens:0,slots:Array.from({length:8},(_,i)=>({slot:i,due:i<2?1:0,opened:i===0?1:0,taken:0,notTaken:0,responseTotalMs:i===0?9000:0,ringTotalMs:i===0?8000:0,responseCount:i===0?1:0,ringCount:i===0?1:0}))};
 const measured=[{id:2,due:846617365,opened:0,responseMs:0,ringMs:1000,reportedAt:0,name:'MED B',slot:1,report:0,flags:4,intervalOpens:2,intervalExtra:1},{id:1,due:846613765,opened:846613774,responseMs:9000,ringMs:8000,reportedAt:0,name:'MED A',slot:0,report:0,flags:3,intervalOpens:0,intervalExtra:0}];
 let dataGeneration=5;
 const context=vm.createContext({document:{getElementById:id=>ids[id],createElement:t=>new Element(t),createTextNode:text=>({textContent:text}),querySelectorAll:()=>all.filter(e=>e.className.includes('needs-link'))},
  console,Date,AbortSignal,URLSearchParams,setInterval(){},fetch:async(path,options)=>{request={path,options};const value=path==='/api/analytics'?metrics:path.startsWith('/api/doses')?{generation:dataGeneration,records:measured,missing:0}:path==='/api/intervals'?{generation:dataGeneration,intervals:[{start:846613765,end:846617365,opens:2,extra:1,partial:0}],missing:0}:response;return {ok:true,json:async()=>structuredClone(value)}}});
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
 fixture.online=true;response=fixture;await vm.runInContext('poll()',context);
 ids.activityFilter.value='all';ids.medicineFilter.value='all';metrics.available=true;
 await vm.runInContext('pollActivity(true)',context);
 assert(ids.statReminders.textContent===2,'real lifetime reminder counts');
 assert(ids.statResponse.textContent==='9s'&&ids.statRing.textContent==='8s','averages exclude pending/empty samples');
 assert(ids.doseList.children.length===2,'measured per-medicine log');
 const textOf=e=>String(e.textContent||'')+(e.children||[]).map(textOf).join(' ');
 assert(textOf(ids.doseList.children[1]).includes('Box opened')&&textOf(ids.doseList.children[1]).includes('Not reported'),'opening never implies medicine taken');
 ids.activityFilter.value='pending';vm.runInContext('renderActivity()',context);assert(ids.doseList.children.length===1&&textOf(ids.doseList).includes('MED B'),'pending filter');
 ids.activityFilter.value='all';vm.runInContext('renderActivity()',context);
 response={operation:43};ids.doseList.children[0].children[2].children[0].onclick();await new Promise(setImmediate);
 assert(request.path==='/api/report'&&request.options.body.get('id')==='2'&&request.options.body.get('previous')==='0','real report button submits ID and expected prior state');
 fixture.operation=43;fixture.result=0;fixture.resultRevision=125;response=fixture;await vm.runInContext('poll()',context);
 assert(vm.runInContext('operation!==null',context),'report ACK alone does not claim success before record readback');
 measured[0].report=1;measured[0].reportedAt=846617400;metrics.slots[1].taken=1;metrics.generation=6;dataGeneration=6;
 await vm.runInContext('pollActivity(true)',context);await vm.runInContext('poll()',context);
 assert(vm.runInContext('operation===null',context)&&ids.message.textContent.includes('report was saved'),'report completes after measured-record readback');
 assert(textOf(ids.doseList.children[0]).includes('Reported taken')&&textOf(ids.doseList.children[0]).includes('35s'),'explicit taken report and report delay displayed');
 dataGeneration=7;await vm.runInContext('pollActivity(true)',context);
 assert(vm.runInContext('activityIncomplete',context),'mixed snapshot generations rejected');
 assert(ids.doseList.children[0].children[2].children[0].disabled,'reports disabled while records are stale');
 return 'PASS: dashboard controls, safe text, revision/ACK flow, offline state, measured averages, honest lid/report separation, filters and generation consistency';
}
