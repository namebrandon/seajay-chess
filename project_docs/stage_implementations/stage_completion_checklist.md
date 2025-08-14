# COMPLETION CHECKLIST AND STEPS TO REVIEW BEFORE MOVING TO A NEW STAGE

## Questions we ask ourselves

If we review the current stage deliverables form the SeaJay Chess Engine Development - Master Project Plan.md document, and our stage-specific implementation plan, have we successfully completed and tested all deliverables?

If we have not completed any deliverables, or only partially completed them, have we noted this in detail in our project_docs/tracking/deferred_items_tracker.md file? If not, we must do so.

Have we identified any bugs from our work on this stage? If so, do they exist in the project_docs/tracking/known_bugs.md file? If not, we must add them using the standard bug template.

If we did any SPRT testing, have we logged the results in project_docs/testing/SPRT_Result_Log.md ? If not, we must do so.

Have we cleaned up any "extra" files such as test scripts, debug scripts, analyze scripts, trace scripts, verify scripts or more that are no longer needed? If we want to keep them, we must document them extensively and put them an appropriate folder in our directory structure, not leave them at the root (unless warranted). If the stage is not complete, we can still keep them temporarily. Please make sure if you remove any files they are only relevant to the stage in question, we have many helper scripts like (yolo.sh and build_macos.sh that are still needed.) Please keep all documentation (.md) files relating to the stage, and make sure they're appropriately organized and cross-referenced in the project_docs/ folder. Documentation dealing with planning goes in project_docs/planning. Documentation relating to implementation and development activity goes in project_docs/stage_implementation. Any major debugging or investigation documents to in project_docs/stage_investigations.


Has the project_docs/project_status.md document been updated recently and with appropriate content? Have we reviewed it for outdated information and removed anything that no longer applies? If not, we should do so.

Has anything significant or materially changed that would impact our README.md file? If so, we should note it.

Is our README.md accurate and up-to-date? If not, we need to make sure it is.

Have we appropriate updated or incremented the version of our engine in its code to reflect a final release for the given stage? We want to be able to tell different builds from different stages from each other. This should be in the UCI class (uci.cpp) as well as any comments or code header.

Is our git repo is a healthy and accurate state? Do we have any maintenance there to perform?

Once we are confident that all of these items are appropriately addressed, we should commit our code with a detailed message for historical reference.
