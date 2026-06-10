// Placeholder for a real photo/screenshot to add later.
// Drop a file in web/public/photos/ and pass src="./photos/mon-image.jpg".
export default function PhotoSlot({ src, alt, caption, ratio = 'aspect-[4/3]' }) {
  if (src) {
    return (
      <figure className="m-0">
        <img
          src={src}
          alt={alt}
          loading="lazy"
          className={`w-full ${ratio} rounded-xl object-cover ring-1 ring-slate-200`}
        />
        {caption && (
          <figcaption className="mt-2 text-center text-sm text-slate-500">{caption}</figcaption>
        )}
      </figure>
    )
  }

  return (
    <div
      className={`flex ${ratio} w-full flex-col items-center justify-center gap-2 rounded-xl border-2 border-dashed border-slate-300 bg-slate-50 p-4 text-center`}
    >
      <svg viewBox="0 0 24 24" className="h-8 w-8 text-slate-400" fill="none" stroke="currentColor" strokeWidth="1.5">
        <rect x="3" y="5" width="18" height="14" rx="2" />
        <circle cx="8.5" cy="10" r="1.5" />
        <path d="M21 16l-5-5-4 4-2-2-7 7" strokeLinecap="round" strokeLinejoin="round" />
      </svg>
      <span className="text-xs font-medium text-slate-400">Photo à ajouter</span>
      {caption && <span className="text-[11px] text-slate-400">{caption}</span>}
    </div>
  )
}
