ZOOTranslate <- function(a) {
    return (.Call("ZOOTranslate",a))
}

ZOOUpdateStatus <- function(a,b) {
    .Call("ZOOUpdateStatus",a,as.numeric(b))
}
