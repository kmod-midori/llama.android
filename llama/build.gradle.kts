@file:Suppress("UnstableApiUsage")

plugins {
    id("com.android.library")
    id("org.jetbrains.kotlin.android")
    id("com.vanniktech.maven.publish")
}

android {
    namespace = "moe.reimu.llama.cpp.android"
    compileSdk = 36

    defaultConfig {
        minSdk = 33

        consumerProguardFiles("consumer-rules.pro")
        ndk {
            abiFilters.clear()
            abiFilters += listOf("arm64-v8a", "x86_64")
        }
        externalNativeBuild {
            cmake {
                arguments += "-DLLAMA_CURL=OFF"
                arguments += "-DLLAMA_BUILD_COMMON=ON"
                arguments += "-DGGML_LLAMAFILE=OFF"
                arguments += "-DCMAKE_BUILD_TYPE=Release"
                cppFlags += listOf()
                arguments += listOf()

                cppFlags("")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    externalNativeBuild {
        cmake {
            path("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }
    kotlinOptions {
        jvmTarget = "1.8"
    }
}

mavenPublishing {
    publishToMavenCentral()
    signAllPublications()

    coordinates("moe.reimu", "llama-cpp-android", "1.0.0-SNAPSHOT")

    pom {
        name.set("llama-cpp-android")
        description.set("Android bindings for llama.cpp")
        inceptionYear.set("2025")
        url.set("https://github.com/kmod-midori/llama.android")
        licenses {
            license {
                name.set("MIT License")
                url.set("https://opensource.org/licenses/MIT")
                distribution.set("https://opensource.org/licenses/MIT")
            }
        }
        developers {
            developer {
                id = "kmod-midori"
                name = "Midori Kochiya"
                url = "https://github.com/kmod-midori/"
            }
        }
        scm {
            url.set("https://github.com/kmod-midori/llama.android")
            connection.set("scm:git:git://github.com/kmod-midori/llama.android.git")
            developerConnection.set("scm:git:ssh://git@github.com/kmod-midori/llama.android.git")
        }
    }
}

dependencies {
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.10.2")
}
