const CACHE_NAME="hwf0910at-pwa-v4";
const APP_SHELL=[
  "/",
  "/index.html",
  "/manifest.webmanifest",
  "/favicon-32.png",
  "/apple-touch-icon.png",
  "/app-icon-192.png",
  "/app-icon-512.png"
];

self.addEventListener("install",event=>{
  event.waitUntil(caches.open(CACHE_NAME).then(cache=>cache.addAll(APP_SHELL)));
  self.skipWaiting();
});

self.addEventListener("activate",event=>{
  event.waitUntil(
    caches.keys()
      .then(keys=>Promise.all(keys.filter(key=>key!==CACHE_NAME).map(key=>caches.delete(key))))
      .then(()=>self.clients.claim())
  );
});

self.addEventListener("fetch",event=>{
  const request=event.request;
  if(request.method!=="GET") return;
  const url=new URL(request.url);
  if(url.origin!==self.location.origin||url.pathname.startsWith("/api/")) return;
  event.respondWith(
    fetch(request,{cache:"no-store"})
      .then(response=>{
        if(response.ok){
          const copy=response.clone();
          caches.open(CACHE_NAME).then(cache=>cache.put(request,copy));
        }
        return response;
      })
      .catch(()=>caches.match(request).then(response=>response||caches.match("/")))
  );
});
