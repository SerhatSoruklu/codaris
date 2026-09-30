// Keep the API same-origin in local development; production calls use the
// dedicated API host. The staff preview is not an account client.
(() => {
  const adminPreview = document.body?.dataset.adminSite === 'true';
  const development = document.body?.dataset.development === 'true';
  const origin = development || adminPreview ? window.location.origin : 'https://api.codaris.org';
  window.codarisApiUrl = path => {
    if (typeof path !== 'string' || !path.startsWith('/api/'))
      throw new TypeError('CODARIS API paths must start with /api/.');
    return new URL(path, origin).href;
  };
})();
