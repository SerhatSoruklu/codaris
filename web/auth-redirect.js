/* Browser-only handoff for the development server's protected-route guard. */
(() => {
  const destination = location.pathname + location.search + location.hash;
  const login = new URL('/login/', location.origin);
  login.searchParams.set('return_to', destination);
  location.replace(login);
})();
