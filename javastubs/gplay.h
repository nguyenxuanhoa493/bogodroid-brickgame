#ifndef __PLAYGAMES_H__
#define __PLAYGAMES_H__

#include "android.h"
#include "baron/baron.h"
#include "javac.h"

void InitJNIGooglePlayClasses(FakeJni::Jvm* vm);

// UnityPlayerActivity appears in erased-overload signatures below. unity.h,
// which defines it, is a project header this shared header cannot include, so
// only the name is needed here; the descriptor (in gplay.cpp) pulls the
// definition.
namespace jnivm {
namespace com {
    namespace unity3d {
        namespace player {
            class UnityPlayerActivity;
        }
    }
}
}

namespace jnivm {
namespace com {
    namespace google {
        namespace android {
            namespace vending {
                namespace licensing {
                    class Obfuscator : public virtual FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/vending/licensing/Obfuscator")
                    };

                    class AESObfuscator : public Obfuscator {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/vending/licensing/AESObfuscator", Obfuscator)
                        AESObfuscator(std::shared_ptr<FakeJni::JByteArray> salt,
                                      std::shared_ptr<FakeJni::JString> packageName,
                                      std::shared_ptr<FakeJni::JString> deviceId);
                    };

                    class Policy : public virtual FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/vending/licensing/Policy")
                        static constexpr jint LICENSED = 0x0100;
                    };

                    class ServerManagedPolicy : public Policy {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/vending/licensing/ServerManagedPolicy", Policy)
                        ServerManagedPolicy(std::shared_ptr<jnivm::android::content::Context> context,
                                            std::shared_ptr<Obfuscator> obfuscator);
                    };

                    class APKExpansionPolicy : public Policy {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/vending/licensing/APKExpansionPolicy", Policy)
                        APKExpansionPolicy(std::shared_ptr<jnivm::android::content::Context> context,
                                           std::shared_ptr<Obfuscator> obfuscator);
                    };

                    class LicenseCheckerCallback : public virtual FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/vending/licensing/LicenseCheckerCallback")
                        virtual void allow(jint reason) = 0;
                        virtual void dontAllow(jint reason) = 0;
                        virtual void applicationError(jint errorCode) = 0;
                    };

                    class LicenseChecker : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/vending/licensing/LicenseChecker")
                        LicenseChecker(std::shared_ptr<jnivm::android::content::Context> context,
                                       std::shared_ptr<Policy> policy,
                                       std::shared_ptr<FakeJni::JString> publicKey);
                        void checkAccess(std::shared_ptr<LicenseCheckerCallback> callback);
                        void onDestroy();
                    };
                }
            }
        }

        namespace android {
            namespace play {
                namespace core {
                    namespace review {
                        class ReviewManager : public FakeJni::JObject {
                        public:
                            DEFINE_CLASS_NAME("com/google/android/play/core/review/ReviewManager")
                        };
                        class ReviewManagerFactory : public FakeJni::JObject {
                        public:
                            DEFINE_CLASS_NAME("com/google/android/play/core/review/ReviewManagerFactory")
                            static std::shared_ptr<ReviewManager> create(std::shared_ptr<jnivm::android::content::Context> context);
                        };
                    }
                    namespace tasks {
                        class OnCompleteListener : public FakeJni::JObject {
                        public:
                            DEFINE_CLASS_NAME("com/google/android/play/core/tasks/OnCompleteListener")
                        };

                        class OnSuccessListener : public FakeJni::JObject {
                        public:
                            DEFINE_CLASS_NAME("com/google/android/play/core/tasks/OnSuccessListener")
                        };

                        class OnFailureListener : public FakeJni::JObject {
                        public:
                            DEFINE_CLASS_NAME("com/google/android/play/core/tasks/OnFailureListener")
                        };

                        class Task : public FakeJni::JObject {
                        public:
                            DEFINE_CLASS_NAME("com/google/android/play/core/tasks/Task")
                            Task(std::shared_ptr<FakeJni::JObject> res = nullptr);
                            std::shared_ptr<FakeJni::JObject> result = nullptr;
                            // Unity's Play Games plugin hands in AndroidJavaProxy
                            // objects typed com.google.android.gms.tasks.*, which
                            // are not the play-core listener classes above. Keep
                            // the listeners as plain Objects and forward by name.
                            std::shared_ptr<FakeJni::JObject> onCompleteListener = nullptr;
                            std::shared_ptr<FakeJni::JObject> onSuccessListener = nullptr;
                            std::shared_ptr<FakeJni::JObject> onFailureListener = nullptr;

                            std::shared_ptr<Task> addOnCompleteListener(std::shared_ptr<FakeJni::JObject> listener);
                            std::shared_ptr<Task> addOnSuccessListener(std::shared_ptr<FakeJni::JObject> listener);
                            std::shared_ptr<Task> addOnFailureListener(std::shared_ptr<FakeJni::JObject> listener);

                            // The fake task is always complete; the plugin reads
                            // these from OnComplete handlers.
                            bool isSuccessful();
                            std::shared_ptr<FakeJni::JObject> getResult();
                        };

                    }
                }
            }

            namespace gms {
                namespace games {
                    class AchievementsClient : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/gms/games/AchievementsClient")
                    };

                    class EventsClient : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/gms/games/EventsClient")
                    };

                    class AuthenticationResult : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/gms/games/AuthenticationResult")
                        // Offline device: the sign-in task succeeds but the
                        // player is not authenticated, so the Play Games plugin
                        // reports "not signed in" and the game proceeds.
                        bool isAuthenticated();
                    };

                    class GamesSignInClient : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/gms/games/GamesSignInClient")
                        std::shared_ptr<jnivm::com::google::android::play::core::tasks::Task> isAuthenticated();
                    };

                    class LeaderboardsClient : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/gms/games/LeaderboardsClient")
                    };

                    class PlayerStatsClient : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/gms/games/PlayerStatsClient")
                    };

                    class PlayersClient : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/gms/games/PlayersClient")
                    };

                    class SnapshotsClient : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/gms/games/SnapshotsClient")
                    };

                    class PlayGames : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/gms/games/PlayGames")
                        static std::shared_ptr<jnivm::com::google::android::gms::games::AchievementsClient> getAchievementsClient(std::shared_ptr<jnivm::android::app::Activity> activity);
                        static std::shared_ptr<jnivm::com::google::android::gms::games::EventsClient> getEventsClient(std::shared_ptr<jnivm::android::app::Activity> activity);
                        static std::shared_ptr<jnivm::com::google::android::gms::games::GamesSignInClient> getGamesSignInClient(std::shared_ptr<jnivm::android::app::Activity> activity);
                        static std::shared_ptr<jnivm::com::google::android::gms::games::LeaderboardsClient> getLeaderboardsClient(std::shared_ptr<jnivm::android::app::Activity> activity);
                        static std::shared_ptr<jnivm::com::google::android::gms::games::PlayerStatsClient> getPlayerStatsClient(std::shared_ptr<jnivm::android::app::Activity> activity);
                        static std::shared_ptr<jnivm::com::google::android::gms::games::PlayersClient> getPlayersClient(std::shared_ptr<jnivm::android::app::Activity> activity);
                        static std::shared_ptr<jnivm::com::google::android::gms::games::SnapshotsClient> getSnapshotsClient(std::shared_ptr<jnivm::android::app::Activity> activity);
                    };

                    class PlayGamesSdk : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/google/android/gms/games/PlayGamesSdk")
                        static void initialize(std::shared_ptr<jnivm::android::content::Context> context);
                    };
                }
            }
        }
    }

    // Google Play Billing.
    //
    // Unity IAP builds its store through these classes in its constructor
    // (newBuilder -> setListener -> enablePendingPurchases -> build). Without
    // them the erased-return lookup yields null and the managed GooglePlay
    // store constructor dies with a NullReferenceException, which Unity
    // retries in a tight loop and never lets the game boot. None of this can
    // work offline, so the stubs return inert objects: build() succeeds,
    // startConnection() never calls back, and every purchase call is a no-op.
    // C++ namespace deliberately does not mirror the Java package: a
    // jnivm::com::android node would shadow jnivm::android for every
    // qualified android::... lookup made inside jnivm::com::* headers.
    namespace billingclient {
        namespace api {
                    class BillingClient;
                    class PendingPurchasesParams;

                    class BillingClientBuilder : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/android/billingclient/api/BillingClient$Builder")
                        std::shared_ptr<BillingClientBuilder> setListener(std::shared_ptr<FakeJni::JObject> listener);
                        std::shared_ptr<BillingClientBuilder> enablePendingPurchases(std::shared_ptr<PendingPurchasesParams> params);
                        std::shared_ptr<BillingClient> build();
                        // Erased spellings. C++ cannot overload on return type
                        // alone, so the Object-returning variants get distinct
                        // C++ names and share the Java name in the descriptor.
                        std::shared_ptr<jnivm::Object> enablePendingPurchasesNoArg();
                        std::shared_ptr<jnivm::Object> buildErased();
                    };

                    class BillingResult : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/android/billingclient/api/BillingResult")
                        jint getResponseCode();
                        jint getOnPurchasesUpdatedSubResponseCode();
                        std::shared_ptr<FakeJni::JString> getDebugMessage();

                        jint responseCode = 3; // BILLING_UNAVAILABLE
                        jint subResponseCode = 0;
                        std::shared_ptr<FakeJni::JString> debugMessage;
                    };

                    class BillingClient : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/android/billingclient/api/BillingClient")
                        static std::shared_ptr<BillingClientBuilder> newBuilder(
                            std::shared_ptr<jnivm::android::content::Context> context);
                        // Unity's AndroidJavaObject bridge erases the declared
                        // activity type to the concrete runtime class and the
                        // builder return to java.lang.Object:
                        //   (Lcom/unity3d/player/UnityPlayerActivity;)Ljava/lang/Object;
                        static std::shared_ptr<jnivm::Object> newBuilder(
                            std::shared_ptr<jnivm::com::unity3d::player::UnityPlayerActivity> activity);
                        void startConnection(std::shared_ptr<FakeJni::JObject> listener);
                        bool isReady();
                        void endConnection();
                        void queryPurchasesAsync(std::shared_ptr<FakeJni::JObject> params,
                                                 std::shared_ptr<FakeJni::JObject> listener);
                        void queryProductDetailsAsync(std::shared_ptr<FakeJni::JObject> params,
                                                      std::shared_ptr<FakeJni::JObject> listener);
                        void acknowledgePurchase(std::shared_ptr<FakeJni::JObject> params,
                                                 std::shared_ptr<FakeJni::JObject> listener);
                        void consumeAsync(std::shared_ptr<FakeJni::JObject> params,
                                          std::shared_ptr<FakeJni::JObject> listener);
                        std::shared_ptr<BillingResult> launchBillingFlow(
                            std::shared_ptr<jnivm::android::app::Activity> activity,
                            std::shared_ptr<FakeJni::JObject> params);
                    };

                    class PendingPurchasesParamsBuilder : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/android/billingclient/api/PendingPurchasesParams$Builder")
                        std::shared_ptr<PendingPurchasesParamsBuilder> enableOneTimeProducts();
                        std::shared_ptr<PendingPurchasesParams> build();
                    };

                    class PendingPurchasesParams : public FakeJni::JObject {
                    public:
                        DEFINE_CLASS_NAME("com/android/billingclient/api/PendingPurchasesParams")
                        static std::shared_ptr<PendingPurchasesParamsBuilder> newBuilder();
                    };
                }
            }
    }
}

#endif
