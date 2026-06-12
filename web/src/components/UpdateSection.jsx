import { useEffect, useState } from 'react'
import { RefreshIcon, WifiIcon, CheckIcon, CursorIcon } from "./icons.jsx";

// "Already installed? Update the card" — for returning users.
// The update happens entirely on the card's own page (served at its IP): it
// already lists the available versions to download AND has the upload form.
// A public HTTPS page cannot push to the card's local HTTP address, so we just
// point the user there rather than duplicating a download button here.
export default function UpdateSection() {
  const [version, setVersion] = useState(null);

  // version.json is written by the CI alongside the build.
  // Absent in local dev -> we just don't show the version badge.
  useEffect(() => {
    fetch("./version.json")
      .then((r) => (r.ok ? r.json() : null))
      .then((d) => d?.version && setVersion(d.version))
      .catch(() => {});
  }, []);

  return (
    <section className="bg-slate-900 py-14 text-slate-100">
      <div className="mx-auto max-w-4xl px-5">
        <div className="text-center">
          <div className="mb-2 inline-flex items-center gap-2 text-sm font-semibold uppercase tracking-wide text-brand-200">
            <RefreshIcon className="h-4 w-4" />
            Déjà installé ?
          </div>
          <h2 className="text-2xl font-bold sm:text-3xl">
            Mettre à jour la carte
          </h2>
          <p className="mx-auto mt-4 max-w-xl text-slate-300">
            Pas besoin de câble : la mise à jour se fait{" "}
            <strong>par WiFi</strong>, depuis la carte elle-même. Votre
            configuration (WiFi, comptes) est <strong>conservée</strong>.
          </p>
          {version && (
            <p className="mt-3 text-sm text-slate-400">
              Dernière version publiée : v{version}
            </p>
          )}
        </div>

        <ol className="mx-auto mt-10 max-w-xl space-y-4">
          <UpdateStep icon={WifiIcon}>
            Sur l’écran de la carte, dans le menu{" "}
            <strong>Paramètres / Informations </strong>repérez son{" "}
            <strong>adresse</strong> (du type
            <code className="mx-1 rounded bg-white/10 px-1.5 py-0.5">
              192.168.x.x
            </code>
            ). Depuis un appareil <em>sur le même WiFi</em>, ouvrez cette
            adresse dans le navigateur.
          </UpdateStep>
          <UpdateStep icon={RefreshIcon}>
            Allez sur la page <strong>Mise à jour</strong> (cliquez sur{" "}
            <strong>Accepter</strong> sur l'écran de la carte pour autoriser
            l'accès).
          </UpdateStep>
          <UpdateStep icon={CursorIcon}>
            Ici, vous verrez la liste des versions disponibles : sélectionnez la
            plus récente puis cliquez sur <strong>Envoyer le binaire</strong>.
          </UpdateStep>
          <UpdateStep icon={CheckIcon}>
            La carte redémarre toute seule, déjà à jour. Rien à reconfigurer. 🎉
          </UpdateStep>
        </ol>

        <p className="mx-auto mt-8 max-w-xl text-center text-xs text-slate-400">
          Astuce : pour <strong>installer une carte neuve</strong>, utilisez
          plutôt le bouton « Flasher ma carte » plus haut (par câble USB).
        </p>
      </div>
    </section>
  );
}

function UpdateStep({ n, icon: Icon, children }) {
  return (
    <li className="flex gap-3">
      {n && (
        <span className="grid h-8 w-8 flex-none place-items-center rounded-full bg-brand-500 text-sm font-bold text-white">
          {n}
        </span>
      )}
      <div className="flex items-start gap-2 pt-1 text-sm text-slate-200">
        <Icon className="h-5 w-5 flex-none text-brand-500" />
        <span>{children}</span>
      </div>
    </li>
  );
}
