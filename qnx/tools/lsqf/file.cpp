#include "file.h"

auto modestr(mode_t mode) -> std::string {
  std::string strmode{};

  if (S_ISFIFO(mode)) {
    strmode += 'p';
  } else if (S_ISCHR(mode)) {
    strmode += 'c';
  } else if (S_ISDIR(mode)) {
    strmode += 'd';
  } else if (S_ISNAM(mode)) {
    strmode += 'n';
  } else if (S_ISBLK(mode)) {
    strmode += 'b';
  } else if (S_ISREG(mode)) {
    strmode += '-';
  } else if (S_ISLNK(mode)) {
    strmode += 'l';
  } else if (S_ISSOCK(mode)) {
    strmode += 's';
  } else {
    strmode += '?';
  }

  strmode += (mode & S_IRUSR) ? 'r' : '-';
  strmode += (mode & S_IWUSR) ? 'w' : '-';
  switch (mode & (S_IXUSR | S_ISUID)) {
    case S_IXUSR:
      strmode += 'x';
      break;
    case S_IXUSR + S_ISUID:
      strmode += 's';
      break;
    case S_ISUID:
      strmode += 'S';
      break;
    default:
      strmode += '-';
      break;
  }
  strmode += (mode & S_IRGRP) ? 'r' : '-';
  strmode += (mode & S_IWGRP) ? 'w' : '-';
  switch (mode & (S_IXGRP | S_ISGID)) {
    case S_IXGRP:
      strmode += 'x';
      break;
    case S_IXGRP + S_ISGID:
      strmode += 's';
      break;
    case S_ISGID:
      strmode += 'S';
      break;
    default:
      strmode += '-';
      break;
  }
  strmode += (mode & S_IROTH) ? 'r' : '-';
  strmode += (mode & S_IWOTH) ? 'w' : '-';
  switch (mode & (S_IXOTH | S_ISVTX)) {
    case S_IXOTH:
      strmode += 'x';
      break;
    case S_IXOTH + S_ISVTX:
      strmode += 't';
      break;
    case S_ISVTX:
      strmode += 'T';
      break;
    default:
      strmode += '-';
      break;
  }

  return strmode;
}
