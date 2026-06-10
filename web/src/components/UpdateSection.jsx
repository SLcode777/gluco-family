import { useEffect, useState } from 'react'
import { DownloadIcon, RefreshIcon, WifiIcon, CheckIcon } from './icons.jsx'

// "Already installed? Update the card" — for returning users.
// The update happens over WiFi via the card's own OTA page (config preserved),
// not over USB. We only provide the latest app .bin to download + instructions.
export default function UpdateSection() {
  const [version, setVersion] = useState(null)

  // version.json is written by the CI next to firmware-update.bin.
  // Absent in local dev -> we just don't show the version badge.
  useEffect(() => {
    fetch('./version.json')
      .then((r) => (r.ok ? r.json() : null))
      .then((d) => d?.version && setVersion(d.version))
      .catch(() => {})
  }, [])

  return (
    <section className="bg-slate-900 py-14 text-slate-100">
      <div className="mx-auto max-w-4xl px-5">
        <div className="text-center">
          <div className="mb-2 inline-flex items-center gap-2 text-sm font-semibold uppercase tracking-wide text-brand-200">
            <RefreshIcon className="h-4 w-4" />
            Déjà installé ?
          </div>
          <h2 className="text-2xl font-bold sm:text-3xl">Mettre à jour la carte</h2>
          <p className="mx-auto mt-4 max-w-xl text-slate-300">
            Pas besoin de câble : la mise à jour se fait <strong>par WiFi</strong>, depuis la carte
            elle-même. Votre configuration (WiFi, comptes) est <strong>conservée</strong>.
          </p>
        </div>

        <div className="mt-10 grid items-start gap-8 sm:grid-cols-2">
          {/* Téléchargement */}
          <div className="rounded-2xl bg-slate-800 p-6 text-center ring-1 ring-white/10">
            <p className="text-sm text-slate-300">Étape 1 — Récupérez le fichier de mise à jour</p>
            <a
              href="./firmware-update.bin"
              download="gluco-family-mise-a-jour.bin"
              className="mt-4 inline-flex items-center gap-3 rounded-2xl bg-brand-500 px-6 py-4 text-base font-semibold text-white shadow-lg shadow-brand-900/40 transition hover:bg-brand-600"
            >
              <DownloadIcon className="h-5 w-5" />
              Télécharger la mise à jour
            </a>
            {version && (
              <p className="mt-3 text-xs text-slate-400">Dernière version disponible : v{version}</p>
            )}
          </div>

          {/* Tuto OTA */}
          <ol className="space-y-4">
            <UpdateStep n="2" icon={WifiIcon}>
              Sur l’écran de la carte, repérez son <strong>adresse</strong> (du type
              <code className="mx-1 rounded bg-white/10 px-1.5 py-0.5">192.168.x.x</code>). Depuis un
              appareil <em>sur le même WiFi</em>, ouvrez cette adresse dans le navigateur.
            </UpdateStep>
            <UpdateStep n="3" icon={RefreshIcon}>
              Allez sur la page <strong>Mise à jour</strong>, choisissez le fichier téléchargé
              (<code className="mx-1 rounded bg-white/10 px-1.5 py-0.5">.bin</code>), puis envoyez.
            </UpdateStep>
            <UpdateStep n="4" icon={CheckIcon}>
              La carte redémarre toute seule, déjà à jour. Rien à reconfigurer. 🎉
            </UpdateStep>
          </ol>
        </div>

        <p className="mx-auto mt-8 max-w-xl text-center text-xs text-slate-400">
          Astuce : pour <strong>installer une carte neuve</strong>, utilisez plutôt le bouton
          « Flasher ma carte » plus haut (par câble USB).
        </p>
      </div>
    </section>
  )
}

function UpdateStep({ n, icon: Icon, children }) {
  return (
    <li className="flex gap-3">
      <span className="grid h-8 w-8 flex-none place-items-center rounded-full bg-brand-500 text-sm font-bold text-white">
        {n}
      </span>
      <div className="flex items-start gap-2 pt-1 text-sm text-slate-200">
        <Icon className="mt-0.5 h-4 w-4 flex-none text-brand-300" />
        <span>{children}</span>
      </div>
    </li>
  )
}
