const addResourcesToCache = async (resources) =>
{
	const cache = await caches.open('v1');
	await cache.addAll(resources);
};

const putInCache = async (request, response) =>
{
	const cache = await caches.open('v1');
	await cache.put(request, response);
};
function shouldcache(name)
{
	if (name.indexOf("/raw/"))
		return false;
	if (name.indexOf("/downloadables"))
		return false;
	return true;
};

const cacheFirst = async ({ request, preloadResponsePromise }) =>
{
	// First try to get the resource from the cache
	const responseFromCache = await caches.match(request);
	if (responseFromCache)
	{
//		console.log("cacheFirst - from cache", request, preloadResponsePromise, responseFromCache);
		return responseFromCache;
	}

	// Next try to use the preloaded response, if it's there
	const preloadResponse = await preloadResponsePromise;
	if (preloadResponse)
	{
//		console.info('using preload response', preloadResponse);
		if (shouldcache(request.url))
			putInCache(request, preloadResponse.clone());
		return preloadResponse;
	}

	// Next try to get the resource from the network
	try
	{
		const responseFromNetwork = await fetch(request.clone());
		// response may be used only once
		// we need to save clone to put one copy in cache
		// and serve second one
		if (shouldcache(request.url))
			putInCache(request, responseFromNetwork.clone());
//		console.info('fetching from network', responseFromNetwork);
		return responseFromNetwork;
	}
	catch (error)
	{
//		console.info('failure', preloadResponse);
		// there is nothing we can do, but we must always
		// return a Response object
		return new Response('Network error happened',
		{
			status: 408,
			headers: { 'Content-Type': 'text/plain' },
		});
	}
};

const enableNavigationPreload = async () =>
{
	if (self.registration.navigationPreload)
	{
		// Enable navigation preloads!
		await self.registration.navigationPreload.enable();
	}
};

self.addEventListener('activate', (event) =>
{
	event.waitUntil(enableNavigationPreload());
});


self.addEventListener('install', (event) =>
{
	event.waitUntil(
		addResourcesToCache([
			'./',
			'./fte_pwa.json',
			'./fte_pwa_sw.js',
			'./ftewebgl.js',
			'./ftewebgl.wasm'
		])
	);
});

self.addEventListener('fetch', (event) =>
{
	event.respondWith(cacheFirst(
	{
		request: event.request,
		preloadResponsePromise: event.preloadResponse
	}));
});
