import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
// Registers the <esp-web-install-button> custom element used in InstallButton.jsx
import 'esp-web-tools'
import App from './App.jsx'
import './index.css'

createRoot(document.getElementById('root')).render(
  <StrictMode>
    <App />
  </StrictMode>,
)
