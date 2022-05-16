#!/bin/bash
set -e

scripts="$(dirname "$0")"

if [ -n "$DOCKER_PASSWORD" ]; then
    echo "$DOCKER_PASSWORD" | docker login -u vxldollarreleaseteam --password-stdin

    # We push this just so it can be a cache next time
    if [[ "$TRAVIS_BRANCH" == "master" || "$TRAVIS_BRANCH" == "docker_cache" ]] && [[ "${TRAVIS_BUILD_STAGE_NAME}" =~ 'Build' ]]; then
        ci_image_name="vxldollarcurrency/vxldollar-env:$TRAVIS_JOB_NAME"
        ci/build-docker-image.sh docker/ci/Dockerfile-$TRAVIS_JOB_NAME "$ci_image_name"
        "$scripts"/custom-timeout.sh 30 docker push "$ci_image_name"
    fi

    if [[ "$TRAVIS_BUILD_STAGE_NAME" == "Master_beta_docker" ]]; then
        # quick build and tag beta network master
        "$scripts"/custom-timeout.sh 30 docker build --build-arg NETWORK=beta --build-arg CI_BUILD=true --build-arg TRAVIS_TAG="$TRAVIS_TAG" -f docker/node/Dockerfile -t vxldollarcurrency/vxldollar-beta:master --cache-from vxldollarcurrency/vxldollar-beta:master .
        "$scripts"/custom-timeout.sh 30 docker push vxldollarcurrency/vxldollar-beta:master
    elif [[ "$TRAVIS_BUILD_STAGE_NAME" =~ "Artifacts" ]]; then
        tags=()
        if [[ "${TRAVIS_TAG}" =~ ("RC"|"DB") ]]; then
            tags+=("$TRAVIS_TAG" latest latest-including-rc)
        elif [ -n "$TRAVIS_TAG" ]; then
            tags+=("$TRAVIS_TAG" latest latest-including-rc)
        elif [ -n "$TRAVIS_BRANCH" ]; then
            tags+=("$TRAVIS_BRANCH")
        fi

        if [[ "$TRAVIS_JOB_NAME" =~ "live" ]]; then
            network_tag_suffix=''
            network="live"
            cached=''
        else
            network_tag_suffix="-beta"
            network="beta"
            # use cache from Master_beta_docker to prevent rebuilds
            cached="--cache-from=vxldollarcurrency/vxldollar-beta:master"
            docker pull vxldollarcurrency/vxldollar-beta:master
        fi

        docker_image_name="vxldollarcurrency/vxldollar${network_tag_suffix}"
        "$scripts"/custom-timeout.sh 30 docker build "$cached" --build-arg NETWORK="$network" --build-arg CI_BUILD=true --build-arg TRAVIS_TAG="$TRAVIS_TAG" -f docker/node/Dockerfile -t "$docker_image_name" .
        for tag in "${tags[@]}"; do
            # Sanitize docker tag
            # https://docs.docker.com/engine/reference/commandline/tag/
            tag="$(printf '%s' "$tag" | tr -c '[a-z][A-Z][0-9]_.-' -)"
            if [ "$tag" != "latest" ]; then
                docker tag "$docker_image_name" "${docker_image_name}:$tag"
            fi
            "$scripts"/custom-timeout.sh 30 docker push "${docker_image_name}:$tag"
        done
    fi
fi
