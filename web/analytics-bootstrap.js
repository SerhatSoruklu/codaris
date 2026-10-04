/* Start the isolated C/Wasm consent controller; Google loading stays in C. */
CodarisAnalytics().catch(() => {
  const status = document.getElementById('analytics-status');
  if (status) status.textContent = 'Analytics is off. These controls could not load; please reload to try again.';
});
