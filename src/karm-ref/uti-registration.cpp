module Karm.Ref;

namespace Karm::Ref {

void Uti::Repository::registerCommonUti() {
    addRegistration(
        PUBLIC_INTENT,
        {
            .name = "public.intent"_sym,
            .description = "Intent"s,
        }
    );
    addRegistration(
        PUBLIC_OPEN,
        {
            .name = "public.open"_sym,
            .description = "Open Action"s,
            .conformsTo = {"public.intent"_sym},
        }
    );
    addRegistration(
        PUBLIC_PREVIEW,
        {
            .name = "public.preview"_sym,
            .description = "Preview Action"s,
            .conformsTo = {"public.intent"_sym},
        }
    );
    addRegistration(
        PUBLIC_MODIFY,
        {
            .name = "public.modify"_sym,
            .description = "Modify Action"s,
            .conformsTo = {"public.intent"_sym},
        }
    );
    addRegistration(
        PUBLIC_SHARE,
        {
            .name = "public.share"_sym,
            .description = "Share Action"s,
            .conformsTo = {"public.intent"_sym},
        }
    );
    addRegistration(
        PUBLIC_PRINT,
        {
            .name = "public.print"_sym,
            .description = "Print Action"s,
            .conformsTo = {"public.intent"_sym},
        }
    );
    addRegistration(
        PUBLIC_ITEM,
        {
            .name = "public.item"_sym,
            .description = "Physical Item"s,
        }
    );
    addRegistration(
        PUBLIC_DIRECTORY,
        {
            .name = "public.directory"_sym,
            .description = "Directory"s,
            .conformsTo = {"public.item"_sym},
        }
    );
    addRegistration(
        PUBLIC_DATA,
        {
            .name = "public.data"_sym,
            .description = "Raw Data"s,
            .conformsTo = {"public.item"_sym},
        }
    );
    addRegistration(
        PUBLIC_TEXT,
        {
            .name = "public.text"_sym,
            .description = "Text Document"s,
            .suffixes = {"txt"s},
            .mimes = {"text/plain"_mime},
            .conformsTo = {"public.data"_sym},
        }
    );
    addRegistration(
        PUBLIC_HTML,
        {
            .name = "public.html"_sym,
            .description = "HTML Document"s,
            .suffixes = {"html"s, "htm"s},
            .mimes = {"text/html"_mime},
            .conformsTo = {"public.text"_sym},
        }
    );
    addRegistration(
        PUBLIC_CSS,
        {
            .name = "public.css"_sym,
            .description = "Cascading Style Sheet"s,
            .suffixes = {"css"s},
            .mimes = {"text/css"_mime},
            .conformsTo = {"public.text"_sym},
        }
    );

    addRegistration(
        PUBLIC_JAVASCRIPT,
        {
            .name = "public.javascript"_sym,
            .description = "JavaScript Source Code"s,
            .suffixes = {"js"s, "mjs"s, "cjs"s},
            .mimes = {"text/javascript"_mime, "application/javascript"_mime},
            .conformsTo = {"public.text"_sym},
        }
    );
    addRegistration(
        PUBLIC_JSON,
        {
            .name = "public.json"_sym,
            .description = "JSON Document"s,
            .suffixes = {"json"s},
            .mimes = {"application/json"_mime},
            .conformsTo = {"public.text"_sym},
        }
    );
    addRegistration(
        PUBLIC_TOML,
        {
            .name = "public.toml"_sym,
            .description = "TOML Configuration File"s,
            .suffixes = {"toml"s},
            .mimes = {"application/toml"_mime},
            .conformsTo = {"public.text"_sym},
        }
    );
    addRegistration(
        PUBLIC_MARKDOWN,
        {
            .name = "public.markdown"_sym,
            .description = "Markdown Document"s,
            .suffixes = {"md"s, "markdown"s, "mdown"s, "mkd"s, "mkdn"s},
            .mimes = {"text/markdown"_mime},
            .conformsTo = {"public.text"_sym},
        }
    );
    addRegistration(
        PUBLIC_XML,
        {
            .name = "public.xml"_sym,
            .description = "XML Document"s,
            .suffixes = {"xml"s},
            .mimes = {"text/xml"_mime, "application/xml"_mime},
            .conformsTo = {"public.text"_sym},
        }
    );
    addRegistration(
        PUBLIC_XHTML,
        {
            .name = "public.xhtml"_sym,
            .description = "XHTML Document"s,
            .suffixes = {"xhtml"s, "xht"s},
            .mimes = {"application/xhtml+xml"_mime},
            .conformsTo = {"public.xml"_sym},
        }
    );

    // Document Types
    addRegistration(
        PUBLIC_PDF,
        {
            .name = "com.adobe.pdf"_sym,
            .description = "PDF Document"s,
            .suffixes = {"pdf"s},
            .mimes = {"application/pdf"_mime},
            .conformsTo = {"public.data"_sym},
        }
    );
    addRegistration(
        PUBLIC_IMAGE,
        {
            .name = "public.image"_sym,
            .description = "Image"s,
            .conformsTo = {"public.data"_sym},
        }
    );
    addRegistration(
        PUBLIC_ICO,
        {
            .name = "public.ico"_sym,
            .description = "Windows Icon"s,
            .suffixes = {"ico"s},
            .mimes = {"image/vnd.microsoft.icon"_mime, "image/x-icon"_mime, "image/ico"_mime},
            .conformsTo = {"public.image"_sym},
        }
    );
    addRegistration(
        PUBLIC_GIF,
        {
            .name = "public.gif"_sym,
            .description = "Gif Image"s,
            .suffixes = {"gif"s},
            .mimes = {"image/gif"_mime},
            .conformsTo = {"public.image"_sym},
        }
    );
    addRegistration(
        PUBLIC_BMP,
        {
            .name = "public.bmp"_sym,
            .description = "Windows Bitmap Image"s,
            .suffixes = {"bmp"s},
            .mimes = {"image/bmp"_mime},
            .conformsTo = {"public.image"_sym},
        }
    );
    addRegistration(
        PUBLIC_PNG,
        {
            .name = "public.png"_sym,
            .description = "Portable Network Graphics"s,
            .suffixes = {"png"s},
            .mimes = {"image/png"_mime},
            .conformsTo = {"public.image"_sym},
        }
    );
    addRegistration(
        PUBLIC_TGA,
        {
            .name = "public.tga"_sym,
            .description = "Truevision TGA Image"s,
            .suffixes = {"tga"s, "icb"s, "vda"s, "vst"s},
            .mimes = {"image/x-tga"_mime},
            .conformsTo = {"public.image"_sym},
        }
    );
    addRegistration(
        PUBLIC_JPEG,
        {
            .name = "public.jpeg"_sym,
            .description = "JPEG Image"s,
            .suffixes = {"jpg"s, "jpeg"s},
            .mimes = {"image/jpeg"_mime},
            .conformsTo = {"public.image"_sym},
        }
    );
    addRegistration(
        PUBLIC_QOI,
        {
            .name = "public.qoi"_sym,
            .description = "Quite OK Image"s,
            .suffixes = {"qoi"s},
            .mimes = {"image/qoi"_mime, "image/x-qoi"_mime},
            .conformsTo = {"public.image"_sym},
        }
    );
    addRegistration(
        PUBLIC_WEBP,
        {
            .name = "public.webp"_sym,
            .description = "Web Image"s,
            .suffixes = {"webp"s},
            .mimes = {"image/webp"_mime},
            .conformsTo = {"public.image"_sym},
        }
    );
    addRegistration(
        PUBLIC_SVG,
        {
            .name = "public.svg"_sym,
            .description = "SVG Image"s,
            .suffixes = {"svg"s},
            .mimes = {"image/svg+xml"_mime},
            .conformsTo = {"public.image"_sym, "public.xml"_sym},
        }
    );
    addRegistration(
        PUBLIC_FONT,
        {
            .name = "public.font"_sym,
            .description = "Font"s,
            .conformsTo = {"public.data"_sym},
        }
    );
    addRegistration(
        PUBLIC_WOFF,
        {
            .name = "public.woff"_sym,
            .description = "Web Font"s,
            .suffixes = {"woff"s},
            .mimes = {"font/woff"_mime},
            .conformsTo = {"public.font"_sym},
        }
    );
    addRegistration(
        PUBLIC_WOFF2,
        {
            .name = "public.woff2"_sym,
            .description = "Web Font Version 2"s,
            .suffixes = {"woff2"s},
            .mimes = {"font/woff2"_mime},
            .conformsTo = {"public.font"_sym},
        }
    );
    addRegistration(
        PUBLIC_TTF,
        {
            .name = "public.ttf"_sym,
            .description = "TrueType Font"s,
            .suffixes = {"ttf"s},
            .mimes = {"font/ttf"_mime},
            .conformsTo = {"public.font"_sym},
        }
    );
    addRegistration(
        PUBLIC_OTF,
        {
            .name = "public.otf"_sym,
            .description = "OpenType Font"s,
            .suffixes = {"otf"s},
            .mimes = {"font/otf"_mime},
            .conformsTo = {"public.font"_sym},
        }
    );
    addRegistration(
        PUBLIC_EOT,
        {
            .name = "public.eot"_sym,
            .description = "Embedded OpenType Font"s,
            .suffixes = {"eot"s},
            .mimes = {"application/vnd.ms-fontobject"_mime},
            .conformsTo = {"public.font"_sym},
        }
    );
    addRegistration(
        PUBLIC_AV,
        {
            .name = "public.av"_sym,
            .description = "Audio Visual"s,
            .conformsTo = {"public.data"_sym},
        }
    );
    addRegistration(
        PUBLIC_MOV,
        {
            .name = "public.mov"_sym,
            .description = "Quicktime Movie"s,
            .suffixes = {"mov"s},
            .mimes = {"applvideo/quicktime"_mime},
            .conformsTo = {"public.av"_sym},
        }
    );
    addRegistration(
        PUBLIC_MP4,
        {
            .name = "public.mp4"_sym,
            .description = "MP4 Video"s,
            .suffixes = {"mp4"s},
            .mimes = {"video/mp4"_mime},
            .conformsTo = {"public.av"_sym},
        }
    );
    addRegistration(
        PUBLIC_WEBM,
        {
            .name = "public.webm"_sym,
            .description = "Web Video"s,
            .suffixes = {"webm"s},
            .mimes = {"video/webm"_mime},
            .conformsTo = {"public.av"_sym},
        }
    );
    addRegistration(
        PUBLIC_MP3,
        {
            .name = "public.mp3"_sym,
            .description = "MP3 Audio"s,
            .suffixes = {"mp3"s},
            .mimes = {"audio/mpeg"_mime, "audio/mp3"_mime},
            .conformsTo = {"public.av"_sym},
        }
    );
    addRegistration(
        PUBLIC_WAV,
        {
            .name = "public.wav"_sym,
            .description = "Waveform Audio"s,
            .suffixes = {"wav"s, "wave"s},
            .mimes = {"audio/wav"_mime, "audio/x-wav"_mime, "audio/vnd.wave"_mime},
            .conformsTo = {"public.av"_sym},
        }
    );
    addRegistration(
        PUBLIC_OGG,
        {
            .name = "public.ogg"_sym,
            .description = "Ogg Audio"s,
            .suffixes = {"ogg"s, "oga"s},
            .mimes = {"audio/ogg"_mime},
            .conformsTo = {"public.av"_sym},
        }
    );
    addRegistration(
        PUBLIC_FLAC,
        {
            .name = "public.flac"_sym,
            .description = "Free Lossless Audio Codec"s,
            .suffixes = {"flac"s},
            .mimes = {"audio/flac"_mime, "audio/x-flac"_mime},
            .conformsTo = {"public.av"_sym},
        }
    );
    addRegistration(
        PUBLIC_AAC,
        {
            .name = "public.aac"_sym,
            .description = "Advanced Audio Coding"s,
            .suffixes = {"aac"s},
            .mimes = {"audio/aac"_mime},
            .conformsTo = {"public.av"_sym},
        }
    );
    addRegistration(
        PUBLIC_ARCHIVE,
        {
            .name = "public.archive"_sym,
            .description = "Archive"s,
            .conformsTo = {"public.data"_sym},
        }
    );
    addRegistration(
        PUBLIC_ZIP,
        {
            .name = "public.zip"_sym,
            .description = "Zip Archive"s,
            .suffixes = {"zip"s},
            .mimes = {"application/zip"_mime},
            .conformsTo = {"public.archive"_sym},
        }
    );
    addRegistration(
        PUBLIC_TAR,
        {
            .name = "public.tar"_sym,
            .description = "Tar Archive"s,
            .suffixes = {"tar"s},
            .mimes = {"application/x-tar"_mime},
            .conformsTo = {"public.archive"_sym},
        }
    );
    addRegistration(
        PUBLIC_GZ,
        {
            .name = "public.gz"_sym,
            .description = "Gunzip Archive"s,
            .suffixes = {"gz"s},
            .mimes = {"application/gzip"_mime},
            .conformsTo = {"public.archive"_sym},
        }
    );
    addRegistration(
        PUBLIC_BZIP2,
        {
            .name = "public.bzip2"_sym,
            .description = "Bzip2 Archive"s,
            .suffixes = {"bz2"s},
            .mimes = {"application/x-bzip2"_mime},
            .conformsTo = {"public.archive"_sym},
        }
    );
    addRegistration(
        PUBLIC_7Z,
        {
            .name = "public.7z"_sym,
            .description = "7Zip Archive"s,
            .suffixes = {"7z"s},
            .mimes = {"application/x-7z-compressed"_mime},
            .conformsTo = {"public.archive"_sym},
        }
    );
    addRegistration(
        PUBLIC_RAR,
        {
            .name = "public.rar"_sym,
            .description = "Rar Archive"s,
            .suffixes = {"rar"s},
            .mimes = {"application/vnd.rar"_mime},
            .conformsTo = {"public.archive"_sym},
        }
    );
    addRegistration(
        PUBLIC_XZ,
        {
            .name = "public.xz"_sym,
            .description = "Xz Archive"s,
            .suffixes = {"xz"s},
            .mimes = {"application/x-xz"_mime},
            .conformsTo = {"public.archive"_sym},
        }
    );
}

} // namespace Karm::Ref