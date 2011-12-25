int main (int argc, char **argv) {
    asm volatile("pause\n": : :"memory");
    return(0);
}

